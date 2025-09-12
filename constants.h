#pragma once

namespace GraphConstants {
    // Pick radius constants for consistent shape() and boundingRect() behavior
    // These values control how easily users can select edges
    constexpr qreal PICK_WIDTH = 20.0;        // Width for QPainterPathStroker in shape()
    constexpr qreal PICK_RADIUS = 10.0;       // Half width for boundingRect() inflation
    
    static_assert(PICK_RADIUS == PICK_WIDTH / 2.0, 
                  "Pick radius must be half of pick width for consistency");
}