-- ezpropkit Example: Battery Level
-- Target: Adafruit RP2040 Prop-Maker Feather
--
-- Features:
--   * Watches voltage divider on GPIO29 (pin A3) - more info see:
-- https://learn.adafruit.com/adafruit-rp2040-prop-maker-feather/power-management
--   * Changes onboard NeoPixel to red/yellow/green to match detected voltage
--   * Plays warning sound when critically low, then shuts off power to speaker/servo
--   * Restores power to speaker/servo when voltage increases

print("Starting battery monitor...")

-- Check battery level
local vbat = prop.battery.voltage()
local pct = prop.battery.percent()
print(string.format("Starting battery Level: %.2fV (%d%%)", vbat, pct))
local timestamp = prop.time.ticks_ms()

-- power on speaker and play middle C
prop.power.enable()
prop.audio.tone(262, 120)
prop.time.sleep_ms(150)

-- init servo
local fuel_gauge = prop.servo.init()
fuel_gauge:angle(180)
prop.time.sleep_ms(1000)
fuel_gauge:angle(0)
prop.time.sleep_ms(1000)
local degrees = 0

-- onboard neopixel at 100% is like LOOKING AT THE SUN
prop.status_pixel.set_brightness(51)

-- Optional: add external neopixels to demo cutoff and to
-- waste power, make the testing go faster
-- local strip = prop.neopixel.init(16)

while true do
    if prop.button.pressed() then
        if prop.power.is_enabled() then
            print("Button pressed, play sound")
            prop.audio.tone(262, 120) -- middle C
            prop.time.sleep_ms(150)
        else
            print("Button pressed but speaker off")
        end
    end
    -- update power status every 2 seconds
    if (prop.time.ticks_ms() - timestamp) > 2000 then
        timestamp = prop.time.ticks_ms()
        vbat = prop.battery.voltage()
        pct = prop.battery.percent()
        print(string.format("Battery Level: %.2fV (%d%%)", vbat, pct))
        -- low battery, oh no!
        if pct < 25 then
            prop.status_pixel.set(255, 0, 0)
            -- alert user and start power saving mode
            if prop.power.is_enabled() then
                prop.audio.tone(196, 100) -- bass G
                prop.time.sleep_ms(150)
                prop.audio.tone(140, 500) -- bass C
                prop.time.sleep_ms(550)
                prop.power.disable();
            end
        else
            -- power restored!
            if not prop.power.is_enabled() then
                prop.power.enable()
            end
            if pct > 75 then
                prop.status_pixel.set(0, 255, 0) -- green
                -- strip:fill(0, 255, 0) -- uncomment for power wasting mode
            elseif pct > 50 then
                prop.status_pixel.set(255, 255, 0) -- yellow
                -- strip:fill(255, 255, 0) -- uncomment for power wasting mode
            else
                prop.status_pixel.set(255, 128, 0) -- orange
                -- strip:fill(255, 128, 0) -- uncomment for power wasting mode
            end
            -- strip:show() -- uncomment for power wasting mode
            degrees = math.floor((pct - 25) * 2.4 + 0.5) -- 100% = 180, 25% = 0
            print(string.format("Fuel gauge angle: %d", degrees))
            fuel_gauge:angle(degrees)
            prop.time.sleep_ms(150)
        end -- check power level
    end -- check timestamp
end -- loop forever
