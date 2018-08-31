// Compresses RGBA color into 32-bit unsigned integer (8 bits per channel)
uint packColorRGBA(vec4 vecColor)
{
    uint packedColor;
    packedColor = uint(vecColor.r * 255.0) & 0xFFu;
    packedColor |= (uint(vecColor.g * 255.0) & 0xFFu) << 8;
    packedColor |= (uint(vecColor.b * 255.0) & 0xFFu) << 16;
    packedColor |= (uint(vecColor.a * 255.0) & 0xFFu) << 24;
    return packedColor;
}

// Decompression equivalent to function above
vec4 unpackColorRGBA(uint packedColor)
{
    vec4 vecColor;
    vecColor.r = float(packedColor & 0xFFu) / 255.0;
    vecColor.g = float((packedColor >> 8)  & 0xFFu) / 255.0;
    vecColor.b = float((packedColor >> 16) & 0xFFu) / 255.0;
    vecColor.a = float((packedColor >> 24) & 0xFFu) / 255.0;
    return vecColor;
}


// --- Functions below used for Hybrid Transparency algorithm ---

// Compresses HDR RGB color with 10 bits per channel (last two bits unused)
uint packColor30bit(vec4 vecColor)
{
    uint packedColor;
    packedColor = uint(vecColor.r * 255.0) & 0x3FFu;
    packedColor |= (uint(vecColor.g * 255.0) & 0x3FFu) << 10;
    packedColor |= (uint(vecColor.b * 255.0) & 0x3FFu) << 20;
    return packedColor;
}

// Decompression equivalent to function above
vec4 unpackColor30bit(uint packedColor)
{
    vec4 vecColor;
    vecColor.r = float(packedColor & 0x3FFu) / 255.0;
    vecColor.g = float((packedColor >> 10)  & 0x3FFu) / 255.0;
    vecColor.b = float((packedColor >> 20) & 0x3FFu) / 255.0;
    vecColor.a = 1.0;
    return vecColor;
}

// Packs accumulated alpha into lower two bytes, fragment count into higher two bytes
uint packAccumAlphaAndFragCount(float accumAlpha, uint fragCount)
{
    uint packedValue = uint(accumAlpha * 255.0) & 0xFFFFu;
    packedValue |= (fragCount << 16) & 0xFFFFu;
    return packedValue;
}

// Decompression equivalent to function above
void unpackAccumAlphaAndFragCount(in uint packedValue, out float accumAlpha, out uint fragCount)
{
    accumAlpha = float(packedValue & 0xFFFFu) / 255.0;
    fragCount = (packedValue >> 16) & 0xFFFFu;
}