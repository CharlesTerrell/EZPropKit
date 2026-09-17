-- ezpropkit Example: Adjust Servo
-- Target: Adafruit RP2040 Prop-Maker Feather
--
-- What it does:
--   * Move servo to 0 degress and announce through speaker
--   * Wait for user to adjust servo arm and click button
--   * Move servo to 180 degress and announce through speaker
--   * Wait for user to check servo arm position and click button
--   * Move servo to 90 degress and announce through speaker
--   * Wait for user to check servo arm position and click button
--   * ...and repeat.
--

print("Starting servo calibration...")

-- power on speaker
prop.power.enable()

-- init servo
local servo_arm = prop.servo.init()

while true do
    servo_arm:angle(180)
    prop.audio.play("n1.mp3")
    prop.time.sleep_ms(705)
    prop.audio.play("hundred.mp3")
    prop.time.sleep_ms(758)
    prop.audio.play("n80.mp3")
    repeat
        prop.time.sleep_ms(10)
    until prop.button.pressed()
    prop.time.sleep_ms(500)

    servo_arm:angle(0)
    prop.audio.play("n0.mp3")
    repeat
        prop.time.sleep_ms(10)
    until prop.button.pressed()
    prop.time.sleep_ms(500)

    servo_arm:angle(90)
    prop.audio.play("n90.mp3")
    repeat
        prop.time.sleep_ms(10)
    until prop.button.pressed()
    prop.time.sleep_ms(500)
end
