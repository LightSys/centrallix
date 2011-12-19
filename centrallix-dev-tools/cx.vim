" Vim syntax file
" Language:     Centrallix
" Creator 	Sjirk Jan
" Last Change:  2011 August 11


" Variable Keywords
syn keyword cxKeywords not action active_background active_bgcolor align allowchars allow_delete allow_modify allow_new allow_nodata allow_obscure allow_query
syn keyword cxKeywords allow_selection allow_view ampm apply_hints_to autoquery autolayout_order auto_destroy auto_focus auto_reset auto_start background baseobj
syn keyword cxKeywords bgcolor borderwidth border_color cellhspacing cellsize cellvspacing checked clickimage click_fgcolor colsep column_width confirm_delete
syn keyword cxKeywords content counter_attribute ctl_type c_message c_to c_from c_date c_viewed datafocus1 datafocus2 data_mode date_only default default_time
syn keyword cxKeywords delay_query_until_reveal deletable deploy_to_client description_fgcolor direction disabledimage disable_color displaymode dragcols
syn keyword cxKeywords empty_description enabled enter_mode epilogue event eventdatefield eventdescfield eventnamefield eventpriofield event_all_params
syn keyword cxKeywords event_cancel event_condition event_confirm event_delay expose_actions_for expose_events_for expose_properties_for fgcolor fgcolor1newrow_bgcolor 
syn keyword cxKeywords fgcolor2 field fieldname field_list find_container fl_width followcurrent font_name font_size form framesize gridinemptyrows hdr_background
syn keyword cxKeywords hdr_bgcolor height highlight highlight_background highlight_bgcolor highlight_fgcolor hrtype icon image inactive_background inactive_bgcolor
syn keyword cxKeywords indicates_activity initialdate inner_border inner_padding is_slave kbdfocus1 kbdfocus2 key_objname keying_method key_fieldname key_1
syn keyword cxKeywords key_2 key_3 key_4 key_5 label label_height label_width location marginwidth master_norecs_action master_null_action maxchars method
syn keyword cxKeywords minpriority modal mode mousefocus1 mousefocus2 msec multienter multiple_instantiation next_form next_form_within newrow_background
syn keyword cxKeywords newrow_bgcolor object objname objectsource object_name ocpm onright osrc outer_border outline_background outline_bgcolor parameter
syn keyword cxKeywords partner_key path pointimage point_fgcolor popup popup_order popup_source popup_text prologue query query_multiselect range readahead
syn keyword cxKeywords readonly receive_updates repeat replicasize revealed_only rowhighlight_background rowhighlight_bgcolor rowheight row1_background row1_bgcolor
syn keyword cxKeywords row2_background row2_bgcolor ruletype scrollahead search_by_range seconds selected selected_index send_updates shadowed shadowx shadowy
syn keyword cxKeywords show_branches show_diagnostics show_root show_root_branch show_selection size source spacing sql style s_motd_id tab_location
syn keyword cxKeywords tab_revealed_only tab_width target target_key_1 target_key_2 target_key_3 target_key_4 target_key_5 text textcolor textcolorhighlight
syn keyword cxKeywords textcolornew title titlebar titlecolor tooltip toplevel tristate type use_having_clause use_3d_lines valign value visible widget_template
syn keyword cxKeywords widget_class width windowsize x y


" Function Keywords
syn keyword cxFunction runserver runclient runstatic abs ascii avg charindex char_length condition
syn keyword cxFunction convert count dateadd datepart escape eval first getdate isnull last lower
syn keyword cxFunction ltrim lztrim max min quote ralign replicate right round rtrim substring sum
syn keyword cxFunction upper user_name wordify

" Numbers
syn match cxNumber '\d\+\.\d*'

" Strings
syn region cxString start='"' end='"'
syn region cxString start='\'' end='\''

" Comments
syn match cxComment "//.*$"

let b:current_syntax = "cmp"

hi def link cxKeywords	PreProc
hi def link cxNumber 	Type
hi def link cxString	Constant
hi def link cxComment	Comment
hi def link cxFunction	Statement
