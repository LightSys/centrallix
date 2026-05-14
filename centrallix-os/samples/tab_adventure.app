$Version=2$
FourTabs "widget/page" {
	title = "Tab Adventure";
	bgcolor = "#c0c0c0";
	x=0; y=0; width=250; height=300;
	
	MainTab "widget/tab" {
		x = 0; y = 0; width=250; height=300;
		tab_location = none;
		
		bgcolor = "#05091dff";
		inactive_bgcolor = "#01010aff";
		selected_index = 1;

		Scene1 "widget/tabpage" {
			height=300;
			fl_width = 100;

			Scene1Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="The Beginning"; }
			Scene1Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					You wake up surounded by the ruins of a village. Houses are burned, walls are colapsed, and roves are caved in.
					The house you're 'in' doesn't even have a roof, and barely half a wall is still standing. 
				";
			}
			Scene1Ask "widget/label" { x=10; y=130; width=250; height=32; font_size=18; fgcolor="#ffc67cff"; text="What do you do?"; }

			Scene1Option1 "widget/textbutton" {
				x = 10; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Get up and investigate the village";
				Scene1Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=2; }
			}

			Scene1Option2 "widget/textbutton" {
				x = 70; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Look around the 'house'";
				Scene1Option2C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=3; }
			}
		}
		
		Scene2 "widget/tabpage" {
			height=300;

			Scene2Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="Exploring"; }
			Scene2Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					You get up and begin to walk around the village. Everything is in pretty bad shape, but a lot of brick fireplaces
					were able to survive decently well.
				";
			}
			Scene2Ask "widget/label" { x=10; y=130; width=250; height=32; font_size=18; fgcolor="#ffc67cff"; text="What do you do?"; }

			Scene2Option1 "widget/textbutton" {
				x = 10; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Leave the village";
				Scene2Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=6; }
			}

			Scene2Option2 "widget/textbutton" {
				x = 70; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Investigate a fireplace";
				Scene2Option2C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=7; }
			}
			
			Scene2Option3 "widget/textbutton" {
				x = 130; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Go back to the first house where you woke up.";
				Scene2Option3C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=3; }
			}
		}
		
		Scene3 "widget/tabpage" {
			height=300;

			Scene3Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="Look around the house"; }
			Scene3Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					You look around the house where you woke up.
				";
			}
			Scene3Ask "widget/label" { x=10; y=130; width=250; height=32; font_size=18; fgcolor="#ffc67cff"; text=""; }

			Scene3Option1 "widget/textbutton" {
				x = 10; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "...";
				Scene3Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=4; }
			}
		}
		
		Scene4 "widget/tabpage" {
			height=300;

			Scene4Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="Food!"; }
			Scene4Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					You find some scraps of food and realize that you're famished!
					The food looks pretty old, though. Maybe it's not a good idea...
				";
			}
			Scene4Ask "widget/label" { x=10; y=130; width=250; height=32; font_size=18; fgcolor="#ffc67cff"; text="What do you do?"; }

			Scene4Option1 "widget/textbutton" {
				x = 10; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Eat the food";
				Scene4Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=5; }
			}
			
			Scene4Option2 "widget/textbutton" {
				x = 70; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Leave it and check out the village";
				Scene4Option2C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=2; }
			}
		}
		
		Scene5 "widget/tabpage" {
			height=300;

			Scene5Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="Bad food..."; }
			Scene5Text1 "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="You eat the food.";
			}
			Scene5Text2 "widget/label" {
				x=10; y=90; width=250; height=80;
				font_size=18; fgcolor="#da2d2dff";
				text="OH NO!!";
			}
			Scene5Text3 "widget/label" {
				x=10; y=120; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="You should not have done that. You feel very sick.";
			}
			Scene5Ask "widget/label" { x=10; y=150; width=250; height=32; font_size=18; fgcolor="#ffc67cff"; text=". . ."; }

			Scene5Option1 "widget/textbutton" {
				x = 10; y = 190; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = ". . .";
				Scene5Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=15; }
			}
		}
		
		Scene6 "widget/tabpage" {
			height=300;

			Scene6Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="FREEDOM"; }
			Scene6Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					You leave the village. As you get further away from the depressing place, you begin to run.
					You sprint across planes and through vallies, never looking back or missing the acursed ruins you left behind.
					After a while, though, you suddenly fall off some kind of edge and plumit until everything goes black...
				";
			}

			Scene6Option1 "widget/textbutton" {
				x = 10; y = 150; width = 75; height = 30; font_size=18; bgcolor="#1a1066ff";
				text = "WHAT?! The writer made the world THAT small?";
				Scene6Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=15; }
			}
		}

		Scene7 "widget/tabpage" {
			height=300;

			Scene7Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="Fireplaces"; }
			Scene7Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					There's several firepalces around, so you have a few options.
				";
			}

			Scene7Option1 "widget/textbutton" {
				x = 10; y = 120; width = 50; height = 24; font_size=18; bgcolor="#0c0447ff";
				text = "Sturdy Fireplace";
				Scene7Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=8; }
			}

			Scene7Option2 "widget/textbutton" {
				x = 10; y = 150; width = 50; height = 24; font_size=18; bgcolor="#0c0447ff";
				text = "Leaning Fireplace";
				Scene7Option2C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=10; }
			}

			Scene7Option3 "widget/textbutton" {
				x = 10; y = 180; width = 50; height = 24; font_size=18; bgcolor="#0c0447ff";
				text = "Crumbling Fireplace";
				Scene7Option3C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=9; }
			}
		}

		Scene8 "widget/tabpage" {
			height=300;

			Scene8Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="'Sturdy' Fireplace"; }
			Scene8Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					You approach the sturdy fireplace and begin to investigate it. Then, you find a loose brick!
					However, as you slowly pull it out, the entire fireplace suddenly topples over onto you.
					Guess it wasn't so sturdy after all!!
				";
			}

			Scene8Option1 "widget/textbutton" {
				x = 10; y = 150; width = 75; height = 30; font_size=18; bgcolor="#660009ff";
				text = "The end";
				Scene8Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=15; }
			}
		}
		
		Scene9 "widget/tabpage" {
			height=300;

			Scene9Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="Crumbling Fireplace"; }
			Scene9Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					You approach the crumbling fireplace and realize it's the one in the house where you woke up!
				";
			}

			Scene9Option1 "widget/textbutton" {
				x = 10; y = 150; width = 75; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Neat!";
				Scene9Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=3; }
			}
		}

		Scene10 "widget/tabpage" {
			height=300;

			Scene10Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="Leaning Fireplace"; }
			Scene10Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					As you investigate the leaning fireplace, you find a loose brick. You carefully slide
					the brick out, and behind it you pull out a golden scroll that seems to call to you.
				";
			}
			Scene10Ask "widget/label" { x=10; y=130; width=250; height=32; font_size=18; fgcolor="#ffc67cff"; text="What do you do?"; }

			Scene10Option1 "widget/textbutton" {
				x = 10; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Read it";
				Scene10Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=11; }
			}

			Scene10Option2 "widget/textbutton" {
				x = 70; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Yikes! Probably cursed. Put it down.";
				Scene10Option2C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=2; }
			}
		}
		
		Scene11 "widget/tabpage" {
			height=300;

			Scene11Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="Leaning Fireplace: Last Chance"; }
			Scene11Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					You know, the scroll probably is cursed.
					Reading it might not be a good idea!
				";
			}
			Scene11Ask "widget/label" { x=10; y=130; width=250; height=32; font_size=18; fgcolor="#ffc67cff"; text="What do you do?"; }

			Scene11Option7 "widget/textbutton" {
				x = 82; y = 230; width = 25; height = 15; font_size=6; fgcolor="#fcc885"; bgcolor="#580251";
				text = "Read it anyway!";
				Scene11Option7C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=12; }
			}

			Scene11Option1 "widget/textbutton" {
				x = 10; y = 180; width = 50; height = 25; font_size=18; bgcolor="#0c0447ff";
				text = "Ok, I'll go investigate the village";
				Scene11Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=2; }
			}

			Scene11Option2 "widget/textbutton" {
				x = 70; y = 180; width = 50; height = 25; font_size=18; bgcolor="#0c0447ff";
				text = "Ok, I'll look around the house where I woke up.";
				Scene11Option2C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=3; }
			}

			Scene11Option3 "widget/textbutton" {
				x = 130; y = 180; width = 50; height = 25; font_size=18; bgcolor="#0c0447ff";
				text = "Ok, I'll go to the sturdy fireplace.";
				Scene11Option3C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=8; }
			}
			
			Scene11Option4 "widget/textbutton" {
				x = 10; y = 220; width = 50; height = 25; font_size=18; bgcolor="#0c0447ff";
				text = "Ok, I'll leave the village.";
				Scene11Option4C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=6; }
			}

			Scene11Option5 "widget/textbutton" {
				x = 70; y = 220; width = 50; height = 25; font_size=18; bgcolor="#1f0757bb";
				text = "Ok, knock myself out.";
				Scene11Option5C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=1; }
			}
			
			Scene11Option6 "widget/textbutton" {
				x = 130; y = 220; width = 50; height = 25; font_size=18; bgcolor="#3e0655ff";
				text = "Ok, I'll die.";
				Scene11Option6C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=15; }
			}
		}
		
		Scene12 "widget/tabpage" {
			height=300;

			Scene12Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="You really shouldn't read that!"; }
			Scene12Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					What? How did you do that! You're not supposed to do that!
				";
			}

			Scene12Option1 "widget/textbutton" {
				x = 10; y = 120; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "JUST LET ME READ THE SCROLL!!!";
				Scene12Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=13; }
			}
		}

		
		Scene13 "widget/tabpage" {
			height=300;

			Scene13Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="VICTORY"; }
			Scene13Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					Ok... fine. You found the golden scroll and won the game.
					Look, I had to make it at least a little bit difficult for you!
				";
			}

			Scene13Option1 "widget/textbutton" {
				x = 10; y = 120; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Yay!";
				Scene13Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=14; }
			}
		}

		
		Scene14 "widget/tabpage" {
			height=300;

			Scene14Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#fff1df"; text="The End"; }
			Scene14Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					Ok... fine. You found the golden scroll and won the game.
					Just thought I'd see if I could trick you there hahahah.
				";
			}
			
			Scene14Text2 "widget/label" {
				x=10; y=120; width=250; height=80;
				font_size=18; fgcolor="#d9e97dff";
				text="
					Thank you for playing!
				";
			}
		}
		
		Scene15 "widget/tabpage" {
			height=300;

			Scene15Title "widget/label" { x=10; y=10; width=250; height=32; font_size=32; fgcolor="#ff001c"; text="DEATH"; }
			Scene15Text "widget/label" {
				x=10; y=50; width=250; height=80;
				font_size=18; fgcolor="#fff1df";
				text="
					You died... Let's call this a learning experience.
					Better luck next time.
				";
			}
			Scene15Ask "widget/label" { x=10; y=130; width=250; height=32; font_size=18; fgcolor="#ffc67cff"; text="What do you do?"; }

			Scene15Option1 "widget/textbutton" {
				x = 10; y = 180; width = 50; height = 30; font_size=18; bgcolor="#0c0447ff";
				text = "Try again?";
				Scene15Option1C "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=1; }
			}
			
			Scene15Option2 "widget/textbutton" { x = 70; y = 180; width = 50; height = 30; font_size=18; bgcolor="#470404ff"; text = "Give up"; }
		}
	}
}
