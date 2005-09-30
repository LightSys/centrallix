$Version=2$
objcanvas_test "widget/template"
    {
    btnFirst "widget/imagebutton"
	{
	widget_class="FirstRecord";
	width=18;
	height=18;
	image="/sys/images/ico16aa.gif";
	pointimage="/sys/images/ico16ab.gif";
	clickimage="/sys/images/ico16ac.gif";
	disabledimage="/sys/images/ico16ad.gif";
	enabled = runclient(:template_form:recid > 1);
	cnFirst "widget/connector" { event="Click"; target=template_form; action="First"; }
	}
    btnBack "widget/imagebutton"
	{
	widget_class="PrevRecord";
	width=18;
	height=18;
	image="/sys/images/ico16ba.gif";
	pointimage="/sys/images/ico16bb.gif";
	clickimage="/sys/images/ico16bc.gif";
	disabledimage="/sys/images/ico16bd.gif";
	enabled = runclient(:template_form:recid > 1);
	cnBack "widget/connector" { event="Click"; target=template_form; action="Prev"; }
	}
    btnNext "widget/imagebutton"
	{
	widget_class="NextRecord";
	width=18;
	height=18;
	image="/sys/images/ico16ca.gif";
	pointimage="/sys/images/ico16cb.gif";
	clickimage="/sys/images/ico16cc.gif";
	disabledimage="/sys/images/ico16cd.gif";
	enabled = runclient(not(:template_form:recid == :template_form:lastrecid));
	cnNext "widget/connector" { event="Click"; target=template_form; action="Next"; }
	}
    btnLast "widget/imagebutton"
	{
	widget_class="LastRecord";
	width=18;
	height=18;
	image="/sys/images/ico16da.gif";
	pointimage="/sys/images/ico16db.gif";
	clickimage="/sys/images/ico16dc.gif";
	disabledimage="/sys/images/ico16dd.gif";
	enabled = runclient(not(:template_form:recid == :template_form:lastrecid));
	cnLast "widget/connector" { event="Click"; target=template_form; action="Last"; }
	}
    btnCtl "widget/textbutton"
	{
	width=60;
	height=21;
	tristate=no;
	fgcolor1=black;
	fgcolor2=white;
	background="/sys/images/grey_gradient.png";
	}
    lbl "widget/label"
	{
	width=80;
	height=20;
	align=right;
	}
    eb "widget/editbox"
	{
	height=20;
	bgcolor=white;
	}
    }
