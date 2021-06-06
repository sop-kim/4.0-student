# 4.0-student

My friend Kim Orna and I wanted to create a chill game that symbolizes what it’s like to be a student during these times, promoting a healthy student lifestyle with a balance between schoolwork, sleep, and drinking lots of water! On a technical note, we wanted to create retro visuals on the VGA display with game elements controlled by user input and several timers, which add randomness to the game whenever you play.

# Devices used (for hardware geeks)
This game uses the DE1-SoC computer with an ARM A9 processor. The environment is simulated on the open source web application CPUlator, graciously created and hosted by Dr. Henry Wong (recent PhD graduate at the University of Toronto). We use:
* VGA Display → both pixel (front and back) and character buffer
* PS/2 Keyboard → use this port to enter keys as described below
* A9 Private Timer
* Interval Timers 1 and 2

# How to set up the game:
* Open the C file in CPUlator https://cpulator.01xz.net/?sys=arm-de1soc 
* Change “Language” to C, then load the file. Click Compile and Load.
* On the right hand bar labelled Devices, find the VGA pixel buffer. Pop this display out onto a new window using the small arrow beside the name and change the zoom level to 1.0, resizing your window as needed for the best experience.
* Find the PS/2 Keyboard I/O bar (with IRQ 79) and click on the entry box labelled “Type here”. Now your keyboard is active; make sure this “Type here” entry is blinking to ensure your keyboard is active during game play.

# Gameplay
* To move the student, tap the left/right/up/down arrows once, and the student will move in the corresponding direction. To stop the student, tap the space key once. Refrain from pressing down keys for too long as it will cause an overflow and stop the game (welcome to the beta version).
* Try to get as many “wellness” icons as possible (each worth 0.4 points), until you can get a GPA of 4.0. These icons include an A+, glass of water, and “zzz”. 
* Your GPA score is displayed in the top-left corner.
* Icons respawn in a new, randomized location every 10 seconds.
* Avoid the phones; they are a source of procrastination! You only have 3 lives, and everytime you hit a phone, your GPA drops by a certain amount. 
* You’ll know you’ve hit a phone when the student’s eyebrows become sad, and you’ll lose a heart in the top-right corner. The phone will respawn elsewhere.
* After you get hit, you have a 3-second grace period where you cannot be affected by a phone again.
* Phones bounce off walls, so be on the lookout!

# Win/Lose
* If you achieve a GPA of 4.0, you’ll see the Con-grad-ulations page, accompanied with a grad cap!
* If you lose all three lives, you’ll see Game Over appear with an encouraging message for the player.
* To play the game again, simply press the enter key, and you’ll be brought to the start of a new game.


Have fun, and don’t forget that while you should strive to be a 4.0 student, life is not all about school!


