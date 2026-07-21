//
//  BLECentral.swift
//  keybug
//
//  Created by john on 7/21/26.
//

import CoreBluetooth
import Observation

// Nordic UART Service — same UUIDs as macos-python/main.py
private let nusService = CBUUID(string: "6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
private let nusRXChar  = CBUUID(string: "6E400002-B5A3-F393-E0A9-E50E24DCCA9E")  // we write here
private let nusTXChar  = CBUUID(string: "6E400003-B5A3-F393-E0A9-E50E24DCCA9E")  // board writes here (unused until Milestone 3)

@Observable
class BLECentral: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    var state: ConnectionState = .idle   // observed by MenuBarView

    @ObservationIgnored private var central: CBCentralManager!
    // CoreBluetooth does not retain discovered peripherals — without this strong
    // reference the connection silently never completes.
    @ObservationIgnored private var peripheral: CBPeripheral?
    @ObservationIgnored private var rxChar: CBCharacteristic?
    @ObservationIgnored private var queue: [Frame] = []

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: nil)  // nil queue = delegate callbacks on main
    }

    // MARK: - Sending

    /// Thread-safe: hops to the main queue, where all CoreBluetooth state lives.
    /// Frames sent while not ready are dropped — this is realtime input, and
    /// replaying stale keystrokes after a reconnect would be worse than losing them.
    func send(_ frame: Frame) {
        DispatchQueue.main.async {
            guard self.state == .ready else { return }
            // TODO: under backpressure, coalesce consecutive mouseMove frames
            // (sum deltas into the queue tail) instead of appending
            self.queue.append(frame)
            self.drainQueue()
        }
    }

    private func drainQueue() {
        guard let peripheral, let rxChar else { return }
        while !queue.isEmpty && peripheral.canSendWriteWithoutResponse {
            let frame = queue.removeFirst()
            peripheral.writeValue(frame.packed, for: rxChar, type: .withoutResponse)
        }
        // If frames remain, peripheralIsReady resumes the drain when the stack has room.
    }

    func peripheralIsReady(toSendWriteWithoutResponse peripheral: CBPeripheral) {
        drainQueue()
    }

    // MARK: - Connection lifecycle

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            startScan()
        } else {
            state = .idle  // powered off / unauthorized / resetting
        }
    }

    private func startScan() {
        state = .scanning
        // Firmware puts the NUS UUID in the advertising packet (name is only in
        // the scan response), so filtering by service is reliable.
        central.scanForPeripherals(withServices: [nusService])
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        self.peripheral = peripheral
        central.stopScan()
        state = .connecting
        central.connect(peripheral)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.delegate = self
        peripheral.discoverServices([nusService])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral,
                        error: Error?) {
        self.peripheral = nil
        startScan()
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let service = peripheral.services?.first(where: { $0.uuid == nusService }) else { return }
        peripheral.discoverCharacteristics([nusRXChar], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService,
                    error: Error?) {
        guard let char = service.characteristics?.first(where: { $0.uuid == nusRXChar }) else { return }
        rxChar = char
        state = .ready
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral,
                        error: Error?) {
        self.peripheral = nil
        rxChar = nil
        queue.removeAll()
        state = .disconnected
        startScan()  // board advertises again on disconnect (restartOnDisconnect), so self-heal
    }
}

enum ConnectionState {
    case idle, scanning, connecting, ready, disconnected
}
