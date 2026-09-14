-- ezpropkit Example: Motion-Activated Sword / Saber Prop
-- Target: Adafruit RP2040 Prop-Maker Feather
--
-- Features:
--   * Blade ignition sound and NeoPixel extension animation
--   * Continuous hum in background
--   * Accelerometer swing detection triggers whoosh sound
--   * Accelerometer clash/tap detection triggers clash sound & flash

print("Igniting Plasma Blade...")

prop.power.enable()

local NUM_LEDS = 30
local blade = prop.neopixel.init(NUM_LEDS)
blade:set_brightness(120)

-- Blade ignition animation
prop.audio.play("ignite.wav", false)
for i = 1, NUM_LEDS do
    blade:set(i, 0, 100, 255) -- Cyan plasma
    blade:show()
    prop.time.sleep_ms(15)
end

-- Play looping idle hum
prop.audio.play("idle_hum.mp3", true)

-- Main motion polling loop
local last_swing = prop.time.ticks_ms()

while true do
    -- 1. Check for physical impact / clash
    if prop.motion.is_tapped() then
        print("CLASH!")
        prop.audio.play("clash.wav", false)
        blade:fill(255, 255, 255) -- White flash
        blade:show()
        prop.time.sleep_ms(80)
        blade:fill(0, 100, 255)
        blade:show()
    end

    -- 2. Check for motion swing
    local x, y, z = prop.motion.read_accel()
    local total_accel = math.sqrt(x*x + y*y + z*z)
    
    -- Normal gravity is ~9.8 m/s^2. Fast swings exceed 22 m/s^2.
    if total_accel > 22.0 and (prop.time.ticks_ms() - last_swing > 500) then
        print("SWING!")
        prop.audio.play("swing.wav", false)
        last_swing = prop.time.ticks_ms()
    end

    prop.time.sleep_ms(25)
end
