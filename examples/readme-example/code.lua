-- AS SEEN IN README.md

-- Simple interactive prop example
prop.led.on()
prop.power.enable() -- Powers 5V boost rail, amp, and external LEDs

-- Initialize 16 external NeoPixels on the terminal block
-- (16 LEDs in my neopixel ring; change to match how many are in yours)
local strip = prop.neopixel.init(16)
strip:fill(0, 100, 255)
strip:show()

-- Play sound file in background
prop.audio.play("powerup.mp3")

-- Listen to the screw-terminal button
while true do
    if prop.button.pressed() then
        prop.audio.play("blaster.wav")
        strip:fill(255, 50, 0)
        strip:show()
        prop.time.sleep_ms(150)
        strip:fill(0, 100, 255)
        strip:show()
    end
    prop.time.sleep_ms(20)
end

