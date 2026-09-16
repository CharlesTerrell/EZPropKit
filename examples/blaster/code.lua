-- ezpropkit Example: Costume Blaster Prop
-- Target: Adafruit RP2040 Prop-Maker Feather
--
-- Features:
--   * Power-up hum on boot
--   * Screw-terminal 'Btn' triggers laser sound effect
--   * NeoPixel muzzle flash animation on 'Neo' terminal
--   * Servo recoil movement on 'Sig' header

print("Starting Blaster Prop...")

-- Enable 5V boost converter (powers amp, NeoPixels, and servo)
prop.power.enable()

-- Initialize 16-pixel NeoPixel muzzle flash on terminal block (GPIO 21)
-- (change it to however many LEDs you have)
local muzzle = prop.neopixel.init(16)
muzzle:set_brightness(150)
muzzle:fill(0, 0, 0)
muzzle:show()

-- Initialize recoil servo on Sig header (GPIO 20)
local recoil_servo = prop.servo.init()
recoil_servo:angle(45)
recoil_servo:angle(0) -- Rest position

-- Play a quick power-up chirp
prop.audio.tone(880, 120)
prop.time.sleep_ms(150)

-- Play ambient hum in background
prop.audio.play("hum.mp3", true) -- Loop = true

local function fire_blaster()
    print("PEW! Blaster fired!")
    
    -- Play laser sound effect
    prop.audio.play("blaster.wav", false)
    
    -- Muzzle flash: Bright orange/cyan flash
    muzzle:fill(255, 120, 20)
    muzzle:show()
    prop.time.sleep_ms(60)
    
    -- Kick servo back for recoil
    recoil_servo:angle(45)
    
    -- Dim muzzle flash
    muzzle:fill(50, 20, 0)
    muzzle:show()
    prop.time.sleep_ms(60)

    -- Return servo to rest
    recoil_servo:angle(0)

    prop.time.sleep_ms(80)
    muzzle:fill(0, 0, 0)
    muzzle:show()
end

-- track time after firing for hum restart
local timestamp = prop.time.ticks_ms()
local humming = true

-- Main prop loop
while true do
    -- Check if trigger button is pressed
    if prop.button.pressed() then
        timestamp = prop.time.ticks_ms()
	humming = false
        fire_blaster()

        -- Let's add a bug! Uncomment the next line and then save:
        --waka waka
        -- It should pulse the onboard neopixel red/orange
        -- and print some debug info to USB serial.

        -- Debounce trigger release
        while prop.button.pressed() do
            prop.time.sleep_ms(10)
        end
    else
        if not humming and (prop.time.ticks_ms() - timestamp) > 500 then
            prop.audio.play("hum.mp3", true)
	    humming = true
        end
    end
    
    prop.time.sleep_ms(20)
end
