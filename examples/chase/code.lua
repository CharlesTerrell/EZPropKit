-- ezpropkit Example: NeoPixel Chase
-- Target: Adafruit RP2040 Prop-Maker Feather
--
-- Features:
--   * Rotates a bright white chase light around a 16-pixel NeoPixel ring
--   * Sets the background ring to the lowest hardware step (1, 1, 1)
--   * Uses full power (255, 255, 255) for the lead pixel to maximize eye contrast
--   * Adds a soft falloff trailing shoulder behind the leader

prop.led.on()
prop.power.enable() -- Powers 5V boost rail and external LEDs

-- Change PIX_COUNT to match how many NeoPixels you have connected
local PIX_COUNT = 16
local strip = prop.neopixel.init(PIX_COUNT)

while true do
    for head = 1, PIX_COUNT do
        -- 1. Fill entire ring with lowest non-zero hardware floor
        strip:fill(1, 1, 1)

        -- 2. Soft falloff shoulder trailing behind the head pixel
        local prev = head - 1
        if head == 1 then
            prev = PIX_COUNT -- wrap around to the last LED
        end
        strip:set(prev, 15, 15, 15)

        -- 3. Lead pixel at full power for maximum contrast ratio
        strip:set(head, 255, 255, 255)

        strip:show()
        prop.time.sleep_ms(150)
    end
end
