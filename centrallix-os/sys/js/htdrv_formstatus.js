// Copyright (C) 1998-2001 LightSys Technology Services, Inc.
//
// You may use these files and this library under the terms of the
// GNU Lesser General Public License, Version 2.1, contained in the
// included file "COPYING" or http://www.gnu.org/licenses/lgpl.txt.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.

function fs_setvalue(m) {
    this.currentMode = m;
    if (this.imagestyle == "large") {
	if (this.currentMode == 'View') {;
		pg_set(pg_images(this)[0],'src','/sys/images/formstatL01.png');
	} else if (this.currentMode == 'Modify') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatL02.png');
	} else if (this.currentMode == 'New') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatL03.png');
	} else if (this.currentMode == 'Query') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatL04.png');
	} else {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatL05.png');
	}
    } else if (this.imagestyle == "largeflat") {
	if (this.currentMode == 'View') {;
		pg_set(pg_images(this)[0],'src','/sys/images/formstatLF01.png');
	} else if (this.currentMode == 'Modify') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatLF02.png');
	} else if (this.currentMode == 'New') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatLF03.png');
	} else if (this.currentMode == 'Query') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatLF04.png');
	} else {
		pg_set(pg_images(this)[0],'src','/sys/images/formstatLF05.png');
	}
    } else {
	if (this.currentMode == 'View') {;
		pg_set(pg_images(this)[0],'src','/sys/images/formstat01.gif');
	} else if (this.currentMode == 'Modify') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstat02.gif');
	} else if (this.currentMode == 'New') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstat03.gif');
	} else if (this.currentMode == 'Query') {
		pg_set(pg_images(this)[0],'src','/sys/images/formstat04.gif');
	} else {
		pg_set(pg_images(this)[0],'src','/sys/images/formstat05.gif');
	}
    }
}

function fs_init(l,s) {
    l.kind = 'formstatus';
    l.mainlayer = l;
    if(cx__capabilities.Dom0NS) {
	l.document.layer = l;
    } else if(cx__capabilities.Dom1HTML) {
	l.layer = l;
    } else {
	alert('browser not supported');
    }
    var images = pg_images(l);
    images[0].layer = l;
    images[0].mainlayer = l;
    images[0].kind = 'formstatus';
    l.currentMode = 'NoData';
    l.isFormStatusWidget = true;
    l.setvalue = fs_setvalue;
    l.imagestyle = s;
    if (fm_current) fm_current.Register(l);
    return l;
}
