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

-- Initialize 8-pixel NeoPixel muzzle flash on terminal block (GPIO 21)
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
    prop.audio.play("laser.wav", false)
    
    -- Muzzle flash: Bright orange/cyan flash
    muzzle:fill(255, 120, 20)
    muzzle:show()
    
    -- Kick servo back for recoil
    recoil_servo:angle(45)
    
    prop.time.sleep_ms(60)
    
    -- Dim muzzle flash
    muzzle:fill(50, 20, 0)
    muzzle:show()
    
    -- Return servo to rest
    recoil_servo:angle(0)
    
    prop.time.sleep_ms(80)
    muzzle:fill(0, 0, 0)
    muzzle:show()
end

-- Main prop loop
while true do
    -- Check if trigger button is pressed
    if prop.button.pressed() then
        fire_blaster()
        
        -- Debounce trigger release
        while prop.button.pressed() do
            prop.time.sleep_ms(10)
        end
    end
    
    prop.time.sleep_ms(20)
end
