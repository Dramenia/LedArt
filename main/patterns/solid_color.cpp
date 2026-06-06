#include "patterns/solid_color.h"
#include "octagon/octagon.h"

void SolidColor::begin(Octagon& octagon) {
    octagon_ = &octagon;
    for (int i = 0; i < 16; i++) octagon.fibers[i].color = color_;
}

void SolidColor::setColor(Color color) {
    color_ = color;
    if (octagon_) {
        for (int i = 0; i < 16; i++) octagon_->fibers[i].color = color_;
    }
}

void SolidColor::update(Octagon& /*octagon*/, uint32_t /*delta_ms*/) {}
