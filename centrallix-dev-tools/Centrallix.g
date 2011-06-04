grammar Centrallix;

options {output=template;}

scope slist {
    List locals; 
    List stats;
}

@header { import org.antlr.stringtemplate.*; }
@lexer::header { import org.antlr.stringtemplate.*; }

program
	:  	version? assignment brace
	;
version
	:	'$Version=2$'
	;
brace
	:	'{' (assignment | brace)* '}'
	;
	
assignment
	:	VARIABLE WIDGET_NAME

	|	(VARIABLE | keywords) '=' (INTEGER | VARIABLE | WIDGET_NAME | function | sql_statement)';'
	|	VARIABLE '=' '\"' (keywords | PATH | expression | '(' expression* ')' )+ '\"' ';'
	|	VARIABLE '=' '\"' '\"' ';'
	|	VARIABLE '=' '\'' (VARIABLE+ | PATH+) '\'' ';'
	;

function // 'not' is not a function, but can be treated as one when followed by parentheses.
	:	( 'runserver' | 'runclient' | 'runstatic' |'abs' | 'ascii' 
	| 'avg' | 'charindex' |	'char_length' | 'condition' | 'convert' 	
   	|	'count'	| 'dateadd' | 'datepart' | 'escape' | 'eval' | 'first' | 'getdate' | 'isnull'	
	| 	'last' | 'lower' | 'ltrim' | 'lztrim' | 'max' | 'min' | 'not' | 'quote' | 'ralign' | 'replicate' 
	| 	'right'| 'round' | 'rtrim' | 'substring' | 'sum' | 'upper' | 'user_name' | 'wordify')
	'(' (expression | function | sql_statement | '\"' expression* '\"')+ ')'	
	;
	

		
sql_statement
	:	'\"' insert? select (from | where | group_by | order_by | having)* '\"' // update delete limit 
	;
	
	insert 
		: 'insert' (expression+ | PATH)
		;
	select
		: 	('select' | 'SELECT') (sql_function | expression | PATH)+
	    ;
	from 
		: ('from' | 'FROM') (expression | PATH)+
		;
	where 
		: 	('where' | 'WHERE') '(' ('\"' | expression | sql_function | PATH)+ ')' ('and' or_and+ | 'or' or_and+ )?
		|	('where' | 'WHERE') (expression | sql_function | PATH)+
		;
		or_and
			:	'(' (expression | sql_function | '\"' | PATH)+ ')'
			|	(sql_function | expression| PATH)
			;
	order_by 
		: 	('order by'| 'ORDER BY') (expression | sql_function | PATH | 'asc' | 'desc')+
		;
	group_by
		:	'group by'(expression+ | PATH)
		;	
	having 
		:	'having' (expression+ | PATH)
		;

	sql_function
		: ( 'avg' | 'charindex' | 'char_length' | 'condition' | 'convert' 	
	   	|	'count'	| 'dateadd' | 'datepart' | 'escape' | 'eval' | 'first' | 'getdate' | 'isnull'	
		| 	'last' | 'lower' | 'ltrim' | 'lztrim' | 'max' | 'min' | 'not' | 'quote' | 'ralign' | 'replicate' 
		| 	'right'| 'round' | 'rtrim' | 'substring' | 'sum' | 'upper' | 'user_name' | 'wordify')
		('(' (expression | sql_function | bracket_expression | PATH )+ ')' | '(' ')' )
		;
bracket_expression
	:	 '(' (expression | PATH)+ ')' 
	;		
expression
	:	VARIABLE | LINE | INTEGER | '\'\\n\'' | '\'\'' | '[' | ']' | '\'' | '+' | ',' 
	| '=' | ';' | '*' | '-' | '/' | 'NOT' | 'AND' | 'OR' | 'IS NULL' | 'IS NOT NULL' | 'or' | 'and'
	| '==' | '!=' | '>' | '>='	|  '<' | '<=' | '*='| '<>' | '!<' | '!>' | 'ASC'|'DESC'
	;

keywords
	:	'first' | 'last' | 'not' | 'asc' | 'desc' | 'from' | 'condition' | 'order' | 'by'
	;
WHITESPACE
    : ( '\t' | ' ' | '\r' | '\n')+ { $channel = HIDDEN;}
    ;   
    
COMMENT
    :   '//' .* '\n' {$channel=HIDDEN;}
    |	'--' .* '\n' {$channel=HIDDEN;}
    |   '/*' .* '*/' {$channel=HIDDEN;}
    ;
VARIABLE // the bracketes could be annoying ... 
    : ('a'..'z'| 'A'..'Z' | '#' | '<' | '>' | '*' | '_')('a'..'z'| 'A'..'Z'|'0'..'9'| '_'| '/' | ':'| '<' | '>' | '*' | '!' | '.')*
    ;
LINE
    : ':'('a'..'z'| 'A'..'Z' |'0'..'9'| '_' | ':')+
    ;
PATH
	:'/' ('a'..'z'| 'A'..'Z'|'0'..'9'| '_'| '/' | '.' | '-')+
	;
INTEGER
	:	('0' .. '9')+
	;
WIDGET_NAME
    : 
('\"widget/autolayout\"' | '\"widget/autolayoutspacer\"' | '\"widget/button\"' | '\"widget/calendar\"' | '\"widget/checkbox\"' | '\"widget/childwindow\"' | '\"wwidget/clock\"' |
'\"widget/component\"' | '\"widget/component-decl\"' | '\"widget/connector\"' | '\"widget/datetime\"' | '\"widget/dropdown\"' | '\"widget/editbox\"' |
'\"widget/execmethod\"' | '\"widget/form\"' | '\"widget/formstatus\"' | '\"widget/frameset\"' | '\"widget/hbox\"' | '\"widget/hints\"' | '\"widget/html\"' |
'\"widget/image\"' | '\"widget/imagebutton\"' | '\"widget/label\"' | '\"widget/menu\"' | '\"widget/osrc\"' |  '\"widget/page\"' | '\"widget/pane\"' |
'\"widget/parameter\"' | '\"widget/radiobuttonpanel\"' | '\"widget/remotectl\"' | '\"widget/remotemgr\"' | '\"widget/repeat\"' | '\"widget/rule\"' |
'\"widget/scrollbar\"' | '\"widget/scrollpane\"' | '\"widget/tab\"' | '\"widget/tabpage\"'  | '\"widget/table\"' | '\"widget/template\"' | '\"widget/textarea\"' |
'\"widget/textbutton\"' | '\"widget/timer\"' | '\"widget/treeview\"' | '\"widget/variable\"' | '\"widget/vbox\"' |  '\"widget/component-decl-event\"' |
'\"widget/component-decl-action\"' | '\"widget/table-column\"')
    ;//list of all the different widgets
	