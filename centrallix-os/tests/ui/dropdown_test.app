//Minsik Lee May 2025 

$Version=2$
dropdown_test "widget/page"
{
    title = "Dropdown Test Application";
    background = "/sys/images/slate2.gif";
    x=0; y=0; width=600; height=800;
    
    Dropdown1 "widget/dropdown"
			{
			mode="dynamic_server";
			x=10; y=10; width=160; height=20;
			hilight="#b5b5b5";
			bgcolor="#c0c0c0";
			fieldname="id";
			numdisplay=10;
			sql="SELECT :full_name,:id FROM /tests/States.csv/rows ORDER BY :full_name";
			}

    Dropdown2 "widget/dropdown"
			{
			mode="dynamic_server";
			x=10; y=110; width=160; height=20;
			hilight="#b5b5b5";
			bgcolor="#c0c0c0";
			fieldname="id";
			numdisplay=10;
      query_multiselect="yes";
			sql="SELECT :full_name,:id FROM /tests/States.csv/rows ORDER BY :full_name";
			}
}