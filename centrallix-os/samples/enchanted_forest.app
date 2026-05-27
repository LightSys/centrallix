$Version=2$
adventure "widget/page" {
	title = "The Enchanted Forest";
	bgcolor = "#0f1a0f";
	x=0; y=0; width=700; height=480;

	MainTab "widget/tab" {
		x=0; y=0; width=700; height=480;
		tab_location=none;
		bgcolor="#0f1a0f";
		inactive_bgcolor="#080d08";
		selected_index=1;

		Scene1 "widget/tabpage" {
			height=480;
			fl_width=100;

			S1Title "widget/label" { x=10; y=10; width=680; height=36; font_size=28; fgcolor="#c8e6c8"; text="Lost"; }
			S1Text "widget/label" {
				x=10; y=55; width=680; height=170;
				font_size=16; fgcolor="#d4edda";
				text="You open your eyes to find yourself standing in an enchanted forest. The ancient trees tower above you, their leaves whispering in a language you almost understand. The sun is sinking fast -- you have maybe an hour of light left. You are at a crossroads. Three paths diverge before you: a misty trail winding to the left, a sunlit opening to the right, and a dark overgrown path straight ahead.";
			}
			S1Ask "widget/label" { x=10; y=230; width=680; height=28; font_size=16; fgcolor="#88bb88"; text="Which path do you take?"; }

			S1Opt1 "widget/textbutton" {
				x=10; y=268; width=220; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Take the misty left path";
				S1C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=2; }
			}
			S1Opt2 "widget/textbutton" {
				x=10; y=308; width=220; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Take the sunlit right path";
				S1C2 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=3; }
			}
			S1Opt3 "widget/textbutton" {
				x=10; y=348; width=220; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Head down the overgrown path";
				S1C3 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=4; }
			}
		}

		Scene2 "widget/tabpage" {
			height=480;

			S2Title "widget/label" { x=10; y=10; width=680; height=36; font_size=28; fgcolor="#c8e6c8"; text="The Misty Path"; }
			S2Text "widget/label" {
				x=10; y=55; width=680; height=160;
				font_size=16; fgcolor="#d4edda";
				text="The mist thickens around you as you walk. Droplets of water cling to your hair and eyelashes. Through the fog you spot a warm amber glow -- a small cottage with smoke curling lazily from its stone chimney. The smell of fresh bread drifts toward you. Someone is home.";
			}
			S2Ask "widget/label" { x=10; y=222; width=680; height=28; font_size=16; fgcolor="#88bb88"; text="What do you do?"; }

			S2Opt1 "widget/textbutton" {
				x=10; y=260; width=220; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Knock on the door";
				S2C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=5; }
			}
			S2Opt2 "widget/textbutton" {
				x=10; y=300; width=220; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Sneak quietly past the cottage";
				S2C2 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=9; }
			}
			S2Opt3 "widget/textbutton" {
				x=10; y=340; width=220; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Turn back to the crossroads";
				S2C3 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=1; }
			}
		}

		Scene3 "widget/tabpage" {
			height=480;

			S3Title "widget/label" { x=10; y=10; width=680; height=36; font_size=28; fgcolor="#c8e6c8"; text="The Sunlit Clearing"; }
			S3Text "widget/label" {
				x=10; y=55; width=680; height=160;
				font_size=16; fgcolor="#d4edda";
				text="The trees open into a beautiful meadow bathed in golden light. Wild berries cluster along the edges -- deep purple and plump. A tall ancient oak stands at the center of the clearing. But then you hear it: a low rumble from the shadows at the far tree line. Something large is watching you.";
			}
			S3Ask "widget/label" { x=10; y=222; width=680; height=28; font_size=16; fgcolor="#88bb88"; text="What do you do?"; }

			S3Opt1 "widget/textbutton" {
				x=10; y=260; width=220; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Climb the tall oak tree";
				S3C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=6; }
			}
			S3Opt2 "widget/textbutton" {
				x=10; y=300; width=220; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Eat some of the wild berries";
				S3C2 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=7; }
			}
			S3Opt3 "widget/textbutton" {
				x=10; y=340; width=220; height=30; font_size=16; bgcolor="#660000";
				text="Run!";
				S3C3 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=9; }
			}
		}

		Scene4 "widget/tabpage" {
			height=480;

			S4Title "widget/label" { x=10; y=10; width=680; height=36; font_size=28; fgcolor="#c8e6c8"; text="The Overgrown Path"; }
			S4Text "widget/label" {
				x=10; y=55; width=680; height=160;
				font_size=16; fgcolor="#d4edda";
				text="Thorns and brambles snag at your clothes as you force your way through. The path has not been walked in years. After struggling through the underbrush, you emerge at the base of a crumbling stone tower covered in ivy. It looks dangerously old. A faded inscription above the doorway reads: VISTA POINT.";
			}
			S4Ask "widget/label" { x=10; y=222; width=680; height=28; font_size=16; fgcolor="#88bb88"; text="What do you do?"; }

			S4Opt1 "widget/textbutton" {
				x=10; y=260; width=220; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Climb the tower";
				S4C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=8; }
			}
			S4Opt2 "widget/textbutton" {
				x=10; y=300; width=220; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Turn back to the crossroads";
				S4C2 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=1; }
			}
		}

		Scene5 "widget/tabpage" {
			height=480;

			S5Title "widget/label" { x=10; y=10; width=680; height=36; font_size=28; fgcolor="#c8e6c8"; text="The Forest Guardian"; }
			S5Text "widget/label" {
				x=10; y=55; width=680; height=210;
				font_size=16; fgcolor="#d4edda";
				text="The door opens slowly. A small, bright-eyed woman with silver hair smiles up at you. 'I wondered when a lost soul would come calling,' she says. She is the guardian of this forest and has helped wanderers for centuries. She presses a lantern of green witch-light into your hands. 'This will light your path home,' she tells you warmly. 'Follow it north. Do not stray.' She also hands you a warm loaf of bread for the journey.";
			}
			S5Ask "widget/label" { x=10; y=272; width=680; height=28; font_size=16; fgcolor="#88bb88"; text="You feel hope for the first time since waking up. What do you do?"; }

			S5Opt1 "widget/textbutton" {
				x=10; y=310; width=270; height=30; font_size=16; bgcolor="#1a4a1a";
				text="Accept the lantern and follow her advice";
				S5C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=11; }
			}
			S5Opt2 "widget/textbutton" {
				x=10; y=350; width=270; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Thank her, but head north on your own";
				S5C2 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=10; }
			}
		}

		Scene6 "widget/tabpage" {
			height=480;

			S6Title "widget/label" { x=10; y=10; width=680; height=36; font_size=28; fgcolor="#c8e6c8"; text="A View From Above"; }
			S6Text "widget/label" {
				x=10; y=55; width=680; height=200;
				font_size=16; fgcolor="#d4edda";
				text="You scramble up the oak's rough bark and climb until you are above the canopy. The whole forest spreads out before you in the fading golden light. Far to the north -- maybe two miles -- you can see the warm glow of a village. You also notice a small cottage to the northwest with smoke drifting from its chimney. The creature below has lost interest and lumbered away.";
			}
			S6Ask "widget/label" { x=10; y=262; width=680; height=28; font_size=16; fgcolor="#88bb88"; text="What do you do?"; }

			S6Opt1 "widget/textbutton" {
				x=10; y=300; width=270; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Climb down and head north toward the village";
				S6C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=10; }
			}
			S6Opt2 "widget/textbutton" {
				x=10; y=340; width=270; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Climb down and visit the cottage first";
				S6C2 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=2; }
			}
		}

		Scene7 "widget/tabpage" {
			height=480;

			S7Title "widget/label" { x=10; y=10; width=680; height=36; font_size=28; fgcolor="#c8e6c8"; text="Enchanted Berries"; }
			S7Text "widget/label" {
				x=10; y=55; width=680; height=210;
				font_size=16; fgcolor="#d4edda";
				text="The berries burst with an extraordinary sweetness. A strange warmth spreads through your body and the world tilts sideways... You wake up. It is dawn. The creature is gone. The enchanted berries have restored your strength -- and done something more. They have given you a strange magical clarity. You know, with perfect certainty, which direction is north.";
			}
			S7Ask "widget/label" { x=10; y=272; width=680; height=28; font_size=16; fgcolor="#88bb88"; text="Time to go home."; }

			S7Opt1 "widget/textbutton" {
				x=10; y=310; width=220; height=30; font_size=16; bgcolor="#1a4a1a";
				text="Head north toward home";
				S7C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=11; }
			}
		}

		Scene8 "widget/tabpage" {
			height=480;

			S8Title "widget/label" { x=10; y=10; width=680; height=36; font_size=28; fgcolor="#c8e6c8"; text="Top of the Tower"; }
			S8Text "widget/label" {
				x=10; y=55; width=680; height=200;
				font_size=16; fgcolor="#d4edda";
				text="The old stairs creak and groan under your feet, but they hold. You emerge onto the crumbling parapet and the forest unfolds below you. Far to the north you can clearly see the lights of a village twinkling through the trees. To the west, a faint glow suggests a small cottage. You have found your bearings at last.";
			}
			S8Ask "widget/label" { x=10; y=262; width=680; height=28; font_size=16; fgcolor="#88bb88"; text="What do you do?"; }

			S8Opt1 "widget/textbutton" {
				x=10; y=300; width=250; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Climb down and head north toward the lights";
				S8C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=10; }
			}
			S8Opt2 "widget/textbutton" {
				x=10; y=340; width=250; height=30; font_size=16; bgcolor="#1a3a1a";
				text="Climb down and find the cottage to the west";
				S8C2 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=2; }
			}
		}

		Scene9 "widget/tabpage" {
			height=480;

			S9Title "widget/label" { x=10; y=10; width=680; height=40; font_size=32; fgcolor="#cc2200"; text="YOU DIED"; }
			S9Text "widget/label" {
				x=10; y=60; width=680; height=200;
				font_size=16; fgcolor="#d4edda";
				text="The enchanted forest is unforgiving to those who are careless. You stumbled in the darkness, or perhaps ran straight toward something you should have avoided. Either way, the forest has claimed another wanderer. The ancient trees close in around you and silence falls. Somewhere deep in the wood, an owl calls once.";
			}
			S9Ask "widget/label" { x=10; y=268; width=680; height=28; font_size=16; fgcolor="#cc8866"; text="Better luck next time."; }

			S9Opt1 "widget/textbutton" {
				x=10; y=308; width=150; height=30; font_size=16; bgcolor="#3a1a1a";
				text="Try again";
				S9C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=1; }
			}
		}

		Scene10 "widget/tabpage" {
			height=480;

			S10Title "widget/label" { x=10; y=10; width=680; height=36; font_size=28; fgcolor="#c8e6c8"; text="The North Path"; }
			S10Text "widget/label" {
				x=10; y=55; width=680; height=200;
				font_size=16; fgcolor="#d4edda";
				text="You find the path heading north and set off with purpose. It goes well at first. But as the last light fades from the sky, the path becomes harder and harder to follow. Tree roots trip you. Branches scratch your face. You stop and stand still in the utter darkness, trying to remember which way is forward. Something is moving in the undergrowth. Very close.";
			}
			S10Ask "widget/label" { x=10; y=262; width=680; height=28; font_size=16; fgcolor="#88bb88"; text="What do you do?"; }

			S10Opt1 "widget/textbutton" {
				x=10; y=300; width=250; height=30; font_size=16; bgcolor="#3a1a1a";
				text="Press on bravely through the darkness";
				S10C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=9; }
			}
			S10Opt2 "widget/textbutton" {
				x=10; y=340; width=250; height=30; font_size=16; bgcolor="#1a4a1a";
				text="Go perfectly still and wait for it to pass";
				S10C2 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=11; }
			}
		}

		Scene11 "widget/tabpage" {
			height=480;

			S11Title "widget/label" { x=10; y=10; width=680; height=40; font_size=32; fgcolor="#f0e68c"; text="YOU MADE IT!"; }
			S11Text "widget/label" {
				x=10; y=60; width=680; height=220;
				font_size=16; fgcolor="#d4edda";
				text="The trees thin and then -- there it is. A village. Real houses with warm lights in real windows. Real people look up from their work as you stumble out of the wood. A blacksmith's apprentice hands you a mug of something hot and asks with great sympathy whether you have been in the forest. You take a long sip and decide the whole story is a little too embarrassing to tell just yet. You are safe.";
			}
			S11Sub "widget/label" {
				x=10; y=290; width=680; height=50;
				font_size=16; fgcolor="#88bb88";
				text="Thank you for playing The Enchanted Forest!";
			}

			S11Opt1 "widget/textbutton" {
				x=10; y=360; width=150; height=30; font_size=16; bgcolor="#1a4a1a";
				text="Play again";
				S11C1 "widget/connector" { event=Click; target=MainTab; action=SetTab; TabIndex=1; }
			}
		}
	}
}
