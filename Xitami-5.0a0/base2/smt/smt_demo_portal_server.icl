<?xml?>
<!--
    Copyright (c) 1996-2009 iMatix Corporation

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at
    your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    For information on alternative licensing for OEMs, please contact
    iMatix Corporation.
 -->
<class
    name    = "smt_demo_portal_server"
    comment = "Demonstration of portal system - server class"
    script  = "smt_object_gen"
    target  = "smt"
    >

<inherit class = "smt_demo_portal_back" />

<method name = "selftest" />

<method name = "print">
    <action>
    icl_console_print ("SERVER PRINT: %s", string);
    smt_demo_portal_response_mogrify (portal, string, NULL, TRUE);
    </action>
</method>

</class>
