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
	:	'{' (subprogram | brace)* '}'
	;
	
subprogram
	:	function
	|	assignment
	;
assignment
	:	VARIABLE WIDGET_NAME
	|	VARIABLE '=' WIDGET_NAME ';'
	|	VARIABLE '=' INTEGER ';'
	|	VARIABLE '=' VARIABLE ';'
	|	VARIABLE '=' function ';'
	|	VARIABLE '=' '\"' PATH '\"' ';'
	;

function
	:	( 'runserver' | 'runclient' | 'runstatic' |'abs' | 'ascii' 
	| 'avg' | 'charindex' |	'char_length' | 'condition' | 'convert' 	
   	|	'count'	| 'dateadd' | 'datepart' | 'escape' | 'eval' | 'first' | 'getdate' | 'isnull'	
	| 	'last' | 'lower' | 'ltrim' | 'lztrim' | 'max' | 'min' | 'quote' | 'ralign' | 'replicate' 
	| 	'right'| 'round' | 'rtrim' | 'substring' | 'sum' | 'upper' | 'user_name' | 'wordify')
	'(' (expression | function | sql_statement )+ ')'	
	;
	

		
sql_statement
	:	'\"' (insert | select | from | where | order_by | having)+ '\"'
	;
	
	insert : 'insert';
	select : 'select' (sql_function | VARIABLE | LINE | '=' | '+' | '\'' | '-' | '\\n'  | ',')+
	    | 	'select' '*'
	    ;
	from 
		: 'from' PATH
		;
	where 
		: 'where' '(' ('\"' | expression | sql_function | '=' )+ ')' 
			('or' '(' ('\"' | expression | sql_function | '=' )+ ')')*
		;
	order_by :'order by';
	having :'having';
// where (:c_from = " + quote(user_name()) + " and :c_to = " + quote() + ") or (:c_from = " + quote(:this:WithWhom) + " and :c_to = " + quote(user_name()) + ")");
	sql_function
		: ( 'avg' | 'charindex' | 'char_length' | 'condition' | 'convert' 	
	   	|	'count'	| 'dateadd' | 'datepart' | 'escape' | 'eval' | 'first' | 'getdate' | 'isnull'	
		| 	'last' | 'lower' | 'ltrim' | 'lztrim' | 'max' | 'min' | 'quote' | 'ralign' | 'replicate' 
		| 	'right'| 'round' | 'rtrim' | 'substring' | 'sum' | 'upper' | 'user_name' | 'wordify')
		('(' (expression | sql_function)+ ')' | '(' ')')	
		;
		
expression
	:	VARIABLE | LINE |  OPERATOR | INTEGER | '\\n' | '\"\\n\"' | '\'' | '[' | ']' | '+' | ',' | 'or' | 'and'
	;		
WHITESPACE
    : ( '\t' | ' ' | '\r' | '\n')+ { $channel = HIDDEN;}
    ;   
    
COMMENT
    :   '//' .* '\n' {$channel=HIDDEN;}
    |   '/*' .* '*/' {$channel=HIDDEN;}
    ;

VARIABLE
    : ('a'..'z'| 'A'..'Z' | '#' )('a'..'z'| 'A'..'Z'|'0'..'9'| '_'| '/')*
    ;
LINE
    : ':'('a'..'z'| 'A'..'Z' |'0'..'9'| '_')+
    ;
PATH
	:'/' ('a'..'z'| 'A'..'Z'|'0'..'9'| '_'| '/' | '.')+
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
('\"widget/autolayout\"' | '\"widget/button\"' | '\"widget/calendar\"' | '\"widget/checkbox\"' | '\"widget/childwindow\"' | '\"wwidget/clock\"' |
'\"widget/component\"' | '\"widget/component-decl\"' | '\"widget/connector\"' | '\"widget/datetime\"' | '\"widget/dropdown\"' | '\"widget/editbox\"' |
'\"widget/execmethod\"' | '\"widget/form\"' | '\"widget/formstatus\"' | '\"widget/frameset\"' | '\"widget/hbox\"' | '\"widget/hints\"' | '\"widget/html\"' |
'\"widget/image\"' | '\"widget/imagebutton\"' | '\"widget/label\"' | '\"widget/menu\"' | '\"widget/osrc\"' |  '\"widget/page\"' | '\"widget/pane\"' |
'\"widget/parameter\"' | '\"widget/radiobuttonpanel\"' | '\"widget/remotectl\"' | '\"widget/remotemgr\"' | '\"widget/repeat\"' | '\"widget/rule\"' |
'\"widget/scrollbar\"' | '\"widget/scrollpane\"' | '\"widget/tab\"' | '\"widget/table\"' | '\"widget/template\"' | '\"widget/textarea\"' |
'\"widget/textbutton\"' | '\"widget/timer\"' | '\"widget/treeview\"' | '\"widget/variable\"' | '\"widget/vbox\"' |  '\"widget/component-decl-event\"' |
'\"widget/component-decl-action\"' | '\"widget/table-column\"')
    ;//list of all the different widgets
	