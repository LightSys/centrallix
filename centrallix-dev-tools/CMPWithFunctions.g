grammar CMPWithFunctions;

options {output=template;}

scope slist {
    List locals; // must be defined one per semicolon
    List stats;
}

@header {
  import org.antlr.stringtemplate.*;
}
@lexer::header {
  import org.antlr.stringtemplate.*;
}
program
    : version? assignment brace
    ;
	version
		: '$Version=2$'
		;    
brace
    : '{' (subprogram | brace)* '}'
    ;

subprogram
    : assignment
    | function
    | COMMENT
    ; 
assignment
    :   'allowchars=' '\"' ALLOWABLE_CHARS '\";'
    |   VARIABLE WIDGET_NAME
    |   VARIABLE '=' parameter 
    |	'order' '=' parameter
    ;

parameter
    :   '\"' (VARIABLE | '(' | ')' )+ '\";'
    |   '\"' (PATHNAME | FILENAME)+ '\";'
    |	'\"' sql_statement '\";'
    |   VARIABLE ';'
    |   function ';'
    |   INTEGER  ';'
    |	'\"' (GENERAL_KEYWORD | 'asc') '\"' ';' 
    |	GENERAL_KEYWORD ';'
    |	'\"\"' ';'
    |	WIDGET_NAME ';'
    ;
    
function // specify any possible arrangements of tokens in each function. Some functions can contain other functions
    :   'abs' 		'(' (VARIABLE | INTEGER)* ')'
    |	'ascii' 	'(' (VARIABLE) ')'
    |	'avg' 		'(' (VARIABLE | INTEGER | ',')* ')'
    |	'charindex' '(' (VARIABLE ',' VARIABLE) ')'
    |	'charlength''(' (VARIABLE) ')'
    |	'condition' '(' (function | OPERATOR | VARIABLE  | LINE)* ')'
    |	'convert' 	'(' (VARIABLE | INTEGER | LINE) ',' (VARIABLE) ')'
    |	'count'		'(' () ')' //WHAT
    |	'dateadd'	'(' () ')' //WHAT?
    |	'datepart'	'(' () ')' //WHAT?
    |	'escape'	'(' () ')' //WHAT?
    |	'eval'		'(' () ')' //WHAT?
    |	'first'		'(' () ')' //WHAT?
    |	'getdate()' //no parameters
    |	'isnull'	'(' (LINE | VARIABLE | INTEGER | ',')* ')'
  	|	'last'		'(' () ')' //WHAT?
  	| 	'lower'		'(' (VARIABLE) ')' //probably "VARIABLE"
  	|	'ltrim'		'(' (VARIABLE) ')' //probably "VARIABLE"
  	|	'lztrim'	'(' (INTEGER) ')'  // represented as a string?
  	|	'max'		'(' () ')' //WHAT?
  	|	'min'		'(' () ')' //SAME WHAT?
  	|	'quote'		'(' (VARIABLE) ')' //NEEDS MORE PARAMETERS
  	|	'ralign'	'(' (VARIABLE ',' INTEGER) ')'
  	|	'replicate' '(' (VARIABLE ',' INTEGER) ')'
  	| 	'right'		'(' (VARIABLE ',' INTEGER) ')'
  	|	'round'		'(' (INTEGER ',' INTEGER) ')'
  	|	'rtrim'		'(' (VARIABLE) ')'
  	|	('runclient(' | 'runclient' '(')  (function | LINE | VARIABLE | INTEGER | '\'' | '\"' | ',')+ ')' //WHATS?
  	|	'runserver(' (function | sql_statement| '\"')* ')'
  	|	'runstatic'	'(' () ')'
	|	'substring'	'(' (VARIABLE ',' INTEGER ',' INTEGER) ')' 
	|	'sum'		'(' () ')' //WHAT?
	|	'upper'		'(' (VARIABLE) ')' 
	|	'user_name()'	//no parameters
	|	'wordify'	'(' (INTEGER) ')'
	;

	
sql_statement 
    : sql_select sql_from sql_where sql_and* sql_order_by*
    ;
	sql_select
		:	 'select' (VARIABLE '=' | '+' | '\'' | '-' | '\\n' | LINE | ',' | sql_function)*
	    | 	'select' '*'
	    ;//select label = :a_ledger_number + ' - ' + :a_ledger_desc, value = :a_ledger_number from /apps/kardia/data/Kardia_DB/a_ledger/rows");
		sql_function
			:	   'abs' 		'(' (VARIABLE | INTEGER)* ')'
    |	'ascii' 	'(' (VARIABLE) ')'
    |	'avg' 		'(' (VARIABLE | INTEGER | ',')* ')'
    |	'charindex' '(' (VARIABLE ',' VARIABLE) ')'
    |	'charlength''(' (VARIABLE) ')'
    |	'condition' '(' (sql_function | OPERATOR | VARIABLE | LINE | INTEGER | 'or' | ',' | '\'' | '\"' | '/' | ':')* ')'
    |	'convert' 	'(' (VARIABLE | INTEGER | LINE) ',' (VARIABLE) ')'
    |	'count'		'(' () ')' //WHAT
    |	'dateadd'	'(' () ')' //WHAT?
    |	'datepart'	'(' (VARIABLE | ',' | sql_function | LINE)+ ')' //WHAT?
    |	'escape'	'(' () ')' //WHAT?
    |	'eval'		'(' () ')' //WHAT?
    |	'first'		'(' () ')' //WHAT?
    |	'getdate()' //no parameters
    |	'isnull'	'(' (LINE | VARIABLE | INTEGER | ',')* ')'
  	|	'last'		'(' () ')' //WHAT?
  	| 	'lower'		'(' (VARIABLE) ')' //probably "VARIABLE"
  	|	'ltrim'		'(' (VARIABLE) ')' //probably "VARIABLE"
  	|	'lztrim'	'(' (INTEGER) ')'  // represented as a string?
  	|	'max'		'(' () ')' //WHAT?
  	|	'min'		'(' () ')' //SAME WHAT?
  	|	'quote'		'(' (VARIABLE | sql_function | LINE*) ')' //NEEDS MORE PARAMETERS
  	|	'ralign'	'(' (VARIABLE ',' INTEGER) ')'
  	|	'replicate' '(' (VARIABLE ',' INTEGER) ')'
  	| 	'right'		'(' (VARIABLE ',' INTEGER) ')'
  	|	'round'		'(' (INTEGER ',' INTEGER) ')'
  	|	'rtrim'		'(' (VARIABLE) ')'
  	|	('runclient(' | 'runclient' '(')  (sql_function | LINE | VARIABLE | INTEGER | '\'' | '\"' | ',')+ ')' //WHATS?
  	|	'runserver(' (sql_function |  '\"')* ')'
  	|	'runstatic'	'(' () ')'
	|	'substring'	'(' (VARIABLE ',' INTEGER ',' INTEGER) ')' 
	|	'sum'		'(' (sql_function | '\'' | '[' | ']' | ':' | LINE | '\\n')+ ')' //WHAT?
	|	'upper'		'(' (VARIABLE) ')' 
	|	'user_name()'	//no parameters
	|	'wordify'	'(' (INTEGER) ')'
			;
	sql_from
	    :	'from' PATHNAME+ FILENAME PATHNAME?
	    ;
	sql_where
	    :	'where' '('(LINE | '=' | sql_function | '\"' | 'and')+ ')' ('or' '('(LINE | '=' | sql_function | '\"' | 'and')+ ')')*
		;
	sql_and
		:	'and' LINE '=' ('\'' VARIABLE '\'' | sql_concatenate)
		;
		sql_concatenate
			:	'\"' '+' VARIABLE '('LINE*')' ('+' '\"')?
			;

	sql_order_by
		:	'order' 'by' LINE+  ('asc' | 'desc')
		;
	
	
WHITESPACE
    : ( '\t' | ' ' | '\r' | '\n' | '?' | '|' | '+')+ { $channel = HIDDEN;}
    ;   
    
COMMENT
    :   '//' .* '\n' {$channel=HIDDEN;}
    |   '/*' .* '*/' {$channel=HIDDEN;}
    ;
    


GENERAL_KEYWORD
    :	'select' | 'from' | 'where' | 'and' | 'order' | 'by'  | 'desc' | 'null' | 'asc'
	;
LINE
    : ':'('a'..'z'| 'A'..'Z'|'0'..'9'| '_')*
    ;
FILENAME
    : '.' ('cmp' | 'gif' | 'Csv' | 'csv' | 'png')
    ;
PATHNAME
    : '/' ('a'..'z'| 'A'..'Z'| '_')*
    ;

VARIABLE
    : ('a'..'z'| 'A'..'Z' | '#' )('a'..'z'| 'A'..'Z'|'0'..'9'| '_'| '/')* (':')?
    ;
INTEGER
    : ('0'..'9')+
    ;
ALLOWABLE_CHARS
    :('a'..'z'| 'A'..'Z'|'0'..'9'| '_')+
    ;
OPERATOR
	: ('+' | '-' | '*' | '/' | 'NOT' | 'AND' | 'OR' | 'IS NULL' | 'IS NOT NULL' | '=' | '==' | '!=' | '>' | '>='
	|  '<' | '<=' | '*='| '<>' | '!<' | '!>')
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
    // Could add dropdown picklist (but that would probably be in the parser)
