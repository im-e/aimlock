#pragma once

struct Color //color definition
{
    float r, g, b, a;
    Color(float _r = 0, float _g = 0, float _b = 0, float _a = 0) : r(_r), g(_g), b(_b), a(_a) {}
    Color operator*(float s) const { return Color(r * s, g * s, b * s, a * s); }
    Color operator+(const Color& c) const { return Color(r + c.r, g + c.g, b + c.b, a + c.a); }
    Color operator*(const Color& c) const { return Color(r * c.r, g * c.g, b * c.b, a * c.a); }
    Color operator/(float s) const { float inv = 1.0f / s; return Color(r * inv, g * inv, b * inv, a * inv); }
};