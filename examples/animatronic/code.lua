-- ezpropkit Example: Animatronic Skull / Creature Prop
-- Target: Adafruit RP2040 Prop-Maker Feather
--
-- Features:
--   * Servo controls jaw / neck movement
--   * Battery voltage check on boot
--   * Periodic roaring sound effect synchronized with jaw movement
--   * NeoPixel glowing eye animation

print("Booting Animatronic Creature...")

-- Check battery level
local vbat = prop.battery.voltage()
local pct = prop.battery.percent()
print(string.format("Battery Level: %.2fV (%d%%)", vbat, pct))

prop.power.enable()

-- 2 NeoPixels for creature eyes
local eyes = prop.neopixel.init(2)
eyes:set_brightness(100)
eyes:fill(255, 0, 0) -- Red eyes
eyes:show()

-- Servo for jaw movement
local jaw = prop.servo.init()
jaw:angle(0) -- Closed

local function roar()
    print("ROAR!")
    prop.audio.play("creature_growl.mp3", false)
    
    -- Open jaw in sync
    for a = 0, 60, 10 do
        jaw:angle(a)
        prop.time.sleep_ms(15)
    end
    
    -- Eye flicker
    for i = 1, 4 do
        eyes:fill(255, 100, 0)
        eyes:show()
        prop.time.sleep_ms(40)
        eyes:fill(255, 0, 0)
        eyes:show()
        prop.time.sleep_ms(40)
    end
    
    -- Close jaw
    for a = 60, 0, -10 do
        jaw:angle(a)
        prop.time.sleep_ms(15)
    end
end

while true do
    -- If button pressed, trigger roar immediately
    if prop.button.pressed() then
        roar()
    end

    -- Idle breathing pulse on eyes
    local t = prop.time.ticks_ms() / 500.0
    local brightness = math.floor((math.sin(t) + 1.0) * 40.0 + 20.0)
    eyes:set_brightness(brightness)
    eyes:fill(255, 0, 0)
    eyes:show()

    prop.time.sleep_ms(30)
end
