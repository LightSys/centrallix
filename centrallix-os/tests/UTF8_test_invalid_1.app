//
//  Centrallix Test Suite
//
//  "UTF8_test_valid.appp"
//  By Noah Board - August 31, 2022
//
//  This app tests the objdrv_struct driver's interactions
//  with invalid UTF-8 characters. 
//  This does not reflect a typical .app file, but does 
//  prove that the objdrv handles invalid chracters properly
//  (the errors being found by mtlexer)
//

$Version=2$
Page1 "widget/page"
    {
    title="Test UTF-8 #00 -�Invalid UTF-8";

    TestField1 "widget/textbutton"
	{
	text="Ведь Бог так полюбил этот";
	Child1 "widget/connector" {text="'жизнь.app'"; }
	Child2 "widget/connector" {text="'index.не'"; }
	Child3 "widget/pane"
	    {
	    Grandchild1 "I'm-making-this-up"
		{
		text="Οὕτως γὰρ ἠγάπησεν ὁ Θεὸς τὸν";
		GreatGrandchild1 "fake/ident" 
		    {
		    text="что";
		    moreText="Ведь Бог так полюбил этот мир, что отдал Своего единственного Сына, чтобы каждый верующий в Него не погиб, но имел вечную жизнь.";
		    }
		}
	    }
	}

    TestField2 "widget/osrc"
	{
	Child1 "widget/pane"
	    {
	    Grandchild1 "I'm-making-this-up"
		{
		text="彼を信ずる者の亡びずして、永遠の生命を得んためなり。";
		GreatGrandchild1 "fake/ident" 
		    {
		    text="世界を愛";
		    moreText="実に神は、ひとり子をさえ惜しまず与えるほどに、この世界を愛してくださいました。それは、神の御子を信じる者が、だれ一人滅びず、永遠のいのちを得るためです。";
		    }
		}
	    }
	Child2 "widget/pane" {text="めです";}
	Child3 "widget/pane" {text="test";}
	}
        
	TestField3 "widget/osrc"
	{
	text="𓀀𓀁𓀂𓀃𓀄𓀅𓀆𓀇𓀈𓀉𓀊𓀋𓀌𓀍𓀎𓀏𓀐𓀑𓀒𓀓𓀔𓀕𓀖𓀗𓀘𓀙𓀚𓀛𓀜𓀝𓀞𓀟𓀠𓀡𓀢𓀣𓀤𓀥𓀦𓀧𓀨𓀩𓀪𓀫𓀬𓀭𓀮𓀯𓀰𓀱𓀲𓀳𓀴𓀵𓀶𓀷𓀸𓀹𓀺𓀻𓀼𓀽𓀾𓀿𓁀𓁁𓁂𓁃𓁄𓁅𓁆𓁇𓁉𓁊𓁋𓁌𓁍𓁎𓁏𓁐𓁑𓁒𓁓𓁔𓁕𓁖𓁗𓁘𓁙𓁚𓁛𓁜𓁝𓁞𓁟𓁠𓁡𓁢𓁣𓁤𓁥𓁦𓁧𓁨𓁩𓁪𓁫𓁬𓁭𓁮𓁯𓁰𓁱𓁲𓁳𓁴𓁵𓁶𓁷𓁸𓁹𓁺𓁻𓁼𓁽𓁾𓁿";
	Child1 "widget/pane" {text="𓂀𓂁𓂂𓂃𓂄";}
	Child2 "widget/pane" {text="𓅀𓅁𓅂𓅃𓅄𓅅𓅆𓅇𓅈";}
	}
    }
