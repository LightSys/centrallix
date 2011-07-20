grammar Centrallix;
// entirely reads through the first 5 (alphabetically) .cmp files in modules/base
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
	|	VARIABLE '=' WIDGET_NAME ';'
	|	VARIABLE '=' INTEGER ';'
	|	VARIABLE '=' VARIABLE ';'
	|	VARIABLE '=' function ';'
	|	VARIABLE '=' '\"' PATH '\"' ';'
	|	VARIABLE '=' '\"' VARIABLE* '\"' ';'
	|	VARIABLE '=' sql_statement ';'
	|	VARIABLE '=' '\"' 'asc' '\"' ';'
	|	VARIABLE '=' '\'' VARIABLE* '\'' ';'
	;

function
	:	( 'runserver' | 'runclient' | 'runstatic' |'abs' | 'ascii' 
	| 'avg' | 'charindex' |	'char_length' | 'condition' | 'convert' 	
   	|	'count'	| 'dateadd' | 'datepart' | 'escape' | 'eval' | 'first' | 'getdate' | 'isnull'	
	| 	'last' | 'lower' | 'ltrim' | 'lztrim' | 'max' | 'min' | 'not' | 'quote' | 'ralign' | 'replicate' 
	| 	'right'| 'round' | 'rtrim' | 'substring' | 'sum' | 'upper' | 'user_name' | 'wordify')
	'(' (expression | function | sql_statement | '\"' expression* '\"')+ ')'	
	;
	

		
sql_statement
	:	'\"' (insert | select | from | where | order_by | having)+ '\"'
	;
	
	insert : 'insert' expression+;
	select : 'select' (sql_function | expression)+
	    | 	'select' '*'
	    ;
	from 
		: 'from' expression+
		;
	where 
		: 	'where' '(' ('\"' | expression | sql_function  )+ ')' 
				('or' '(' ('\"' | expression | sql_function  )+ ')')*
		|	'where' (expression | sql_function)+
		;
	order_by 
		:'order by' expression* ('asc' | 'desc')?
		;

	having 
		:'having' expression+
		;

	sql_function
		: ( 'avg' | 'charindex' | 'char_length' | 'condition' | 'convert' 	
	   	|	'count'	| 'dateadd' | 'datepart' | 'escape' | 'eval' | 'first' | 'getdate' | 'isnull'	
		| 	'last' | 'lower' | 'ltrim' | 'lztrim' | 'max' | 'min' | 'not' | 'quote' | 'ralign' | 'replicate' 
		| 	'right'| 'round' | 'rtrim' | 'substring' | 'sum' | 'upper' | 'user_name' | 'wordify')
		('(' (expression | sql_function | quoted_bracket)+ ')' | '(' ')')	
		;
		
expression
	:	VARIABLE | LINE |  OPERATOR | INTEGER | PATH | '\'\\n\'' | '\'' | '\'\'' | '[' | ']' 
	| '+' | ',' | 'or' | 'and' | '=' '*'? | '-'
	;
quoted_bracket
	:	'\'(\'' | '\')\''
	;
WHITESPACE
    : ( '\t' | ' ' | '\r' | '\n')+ { $channel = HIDDEN;}
    ;   
    
COMMENT
    :   '//' .* '\n' {$channel=HIDDEN;}
    |   '/*' .* '*/' {$channel=HIDDEN;}
    ;

VARIABLE
    : ('a'..'z'| 'A'..'Z' | '#' | '<' | '>')('a'..'z'| 'A'..'Z'|'0'..'9'| '_'| '/' | ':'| '<' | '>')*
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
OPERATOR
	: '+' | '-' | '*' | '/' | 'NOT' | 'AND' | 'OR' | 'IS NULL' | 'IS NOT NULL' | '=' | '==' | '!=' | '>' | '>='
	|  '<' | '<=' | '*='| '<>' | '!<' | '!>'
	;
WIDGET_NAME
    : 
('\"widget/autolayout\"' | '\"widget/autolayoutspacer\"' | '\"widget/button\"' | '\"widget/calendar\"' | '\"widget/checkbox\"' | '\"widget/childwindow\"' | '\"wwidget/clock\"' |
'\"widget/component\"' | '\"widget/component-decl\"' | '\"widget/connector\"' | '\"widget/datetime\"' | '\"widget/dropdown\"' | '\"widget/editbox\"' |
'\"widget/execmethod\"' | '\"widget/form\"' | '\"widget/formstatus\"' | '\"widget/frameset\"' | '\"widget/hbox\"' | '\"widget/hints\"' | '\"widget/html\"' |
'\"widget/image\"' | '\"widget/imagebutton\"' | '\"widget/label\"' | '\"widget/menu\"' | '\"widget/osrc\"' |  '\"widget/page\"' | '\"widget/pane\"' |
'\"widget/parameter\"' | '\"widget/radiobuttonpanel\"' | '\"widget/remotectl\"' | '\"widget/remotemgr\"' | '\"widget/repeat\"' | '\"widget/rule\"' |
'\"widget/scrollbar\"' | '\"widget/scrollpane\"' | '\"widget/tab\"' | '\"widget/table\"' | '\"widget/template\"' | '\"widget/textarea\"' |
'\"widget/textbutton\"' | '\"widget/timer\"' | '\"widget/treeview\"' | '\"widget/variable\"' | '\"widget/vbox\"' |  '\"widget/component-decl-event\"' |
'\"widget/component-decl-action\"' | '\"widget/table-column\"')
    ;//list of all the different widgets
	