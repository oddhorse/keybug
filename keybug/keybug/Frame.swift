//
//  Frame.swift
//  keybug
//
//  Created by john on 7/20/26.
//

import SwiftUI

struct Frame {
    var eventType: EventType
    var code: UInt8 = 0
    var value: Int16 = 0
    var value2: Int16 = 0
    var modifiers: UInt8 = 0
    var packed: Data {
        var d = Data(capacity: 7)
        d.append(eventType.rawValue)
        d.append(code)
        d.append(UInt8(truncatingIfNeeded: value))        // low byte first = little-endian
        d.append(UInt8(truncatingIfNeeded: value >> 8))   // then high byte
        d.append(UInt8(truncatingIfNeeded: value2))        // low byte first = little-endian
        d.append(UInt8(truncatingIfNeeded: value2 >> 8))   // then high byte
        d.append(modifiers)
        // ... code, value, value2, modifiers, in wire order
        return d
    }
}

enum EventType: UInt8 {
    case keyDown = 0x01
    case keyUp = 0x02
    case mouseMove = 0x03
    case mouseBtn = 0x04
    case scroll = 0x05
    case consumerDown = 0x06
    case consumerUp = 0x07
    case clear = 0x08   // firmware releases all held keys/modifiers
}

