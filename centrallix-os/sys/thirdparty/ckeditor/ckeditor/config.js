/**
 * @license Copyright (c) 2003-2018, CKSource - Frederico Knabben. All rights reserved.
 * For licensing, see https://ckeditor.com/legal/ckeditor-oss-license
 */

CKEDITOR.editorConfig = function( config ) {
	// Define changes to default configuration here. For example:
	// config.language = 'fr';
	// config.uiColor = '#AADC6E';


//probably should not change this, keeps user interface standard
        config.toolbarGroups = [
    { name: 'document',    groups: [ 'mode', 'document', 'doctools' ] },
    { name: 'clipboard',   groups: [ 'clipboard', 'undo' ] },
    { name: 'editing',     groups: [ 'find', 'selection', 'spellchecker' ] },
    { name: 'forms' },
    { name: 'basicstyles', groups: [ 'basicstyles', 'cleanup' ] },
    { name: 'paragraph',   groups: [ 'list', 'indent', 'blocks', 'align', 'bidi' ] },
    { name: 'links' },
    { name: 'insert' },
    '/',
    { name: 'styles' },
    { name: 'colors' },
    { name: 'tools' },
    { name: 'others' },
    { name: 'about' }
];
        //removes unnessecary features
        config.removeButtons='Subscript,Superscript,BidiLtr,BidiRtl,Anchor,Language,Source,Preview,PasteText,PasteFromWord,Find,Replace,ShowBlocks,Format,Styles';
};
