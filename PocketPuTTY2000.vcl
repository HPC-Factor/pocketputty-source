<html>
<body>
<pre>
<h1>Build Log</h1>
<h3>
--------------------Configuration: PocketPuTTY2000 - Win32 (WCE x86) Debug--------------------
</h3>
<h3>Command Lines</h3>
Creating temporary file "C:\DOCUME~1\Aleq\LOCALS~1\Temp\RSP5AD.tmp" with contents
[
/nologo /W3 /Zi /Od /D "DEBUG" /D "_i386_" /D UNDER_CE=300 /D _WIN32_WCE=300 /D "WIN32_PLATFORM_PSPC=310" /D "i_386_" /D "UNICODE" /D "_UNICODE" /D "_X86_" /D "x86" /FR"X86Dbg/" /Fp"X86Dbg/PocketPuTTY2000.pch" /YX /Fo"X86Dbg/" /Fd"X86Dbg/" /Gs8192 /GF /c 
"D:\Devel\Projects\PocketPuTTY\winnet.c"
]
Creating command line "cl.exe @C:\DOCUME~1\Aleq\LOCALS~1\Temp\RSP5AD.tmp" 
Creating temporary file "C:\DOCUME~1\Aleq\LOCALS~1\Temp\RSP5AE.tmp" with contents
[
commctrl.lib coredll.lib corelibc.lib aygshell.lib winsock.lib Iphlpapi.lib /nologo /base:"0x00010000" /stack:0x10000,0x1000 /entry:"WinMainCRTStartup" /incremental:yes /pdb:"X86Dbg/PocketPuTTY2000.pdb" /debug /nodefaultlib:"OLDNAMES.lib" /nodefaultlib:libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib /out:"X86Dbg/PocketPuTTY2000.exe" /subsystem:windowsce,3.00 /MACHINE:IX86 
.\X86Dbg\_devel_test.obj
.\X86Dbg\be_all.obj
.\X86Dbg\cmdline.obj
.\X86Dbg\config.obj
.\X86Dbg\dialog.obj
.\X86Dbg\ldisc.obj
.\X86Dbg\ldiscucs.obj
.\X86Dbg\logging.obj
.\X86Dbg\misc.obj
.\X86Dbg\noise.obj
.\X86Dbg\pageantc.obj
.\X86Dbg\portfwd.obj
.\X86Dbg\porting.obj
.\X86Dbg\pproxy.obj
.\X86Dbg\proxy.obj
.\X86Dbg\raw.obj
.\X86Dbg\rlogin.obj
.\X86Dbg\settings.obj
.\X86Dbg\sizetip.obj
.\X86Dbg\ssh.obj
.\X86Dbg\sshaes.obj
.\X86Dbg\sshblowf.obj
.\X86Dbg\sshbn.obj
.\X86Dbg\sshcrc.obj
.\X86Dbg\sshcrcda.obj
.\X86Dbg\sshdes.obj
.\X86Dbg\sshdh.obj
.\X86Dbg\sshdss.obj
.\X86Dbg\sshmd5.obj
.\X86Dbg\sshpubk.obj
.\X86Dbg\sshrand.obj
.\X86Dbg\sshrsa.obj
.\X86Dbg\sshsh512.obj
.\X86Dbg\sshsha.obj
.\X86Dbg\sshzlib.obj
.\X86Dbg\telnet.obj
.\X86Dbg\terminal.obj
.\X86Dbg\tree234.obj
.\X86Dbg\unicode.obj
.\X86Dbg\version.obj
.\X86Dbg\wcemisc.obj
.\X86Dbg\wcestuff.obj
.\X86Dbg\wcetime.obj
.\X86Dbg\wcwidth.obj
.\X86Dbg\wildcard.obj
.\X86Dbg\wincfg.obj
.\X86Dbg\winctrls.obj
.\X86Dbg\windefs.obj
.\X86Dbg\windlg.obj
.\X86Dbg\window.obj
.\X86Dbg\winmisc.obj
.\X86Dbg\winnet.obj
.\X86Dbg\winstore.obj
.\X86Dbg\winutils.obj
.\X86Dbg\x11fwd.obj
.\X86Dbg\wce_res.res
]
Creating command line "link.exe @C:\DOCUME~1\Aleq\LOCALS~1\Temp\RSP5AE.tmp"
<h3>Output Window</h3>
Compiling...
winnet.c
Linking...
Creating temporary file "C:\DOCUME~1\Aleq\LOCALS~1\Temp\RSP5AF.tmp" with contents
[
/nologo /o"X86Dbg/PocketPuTTY2000.bsc" 
.\X86Dbg\_devel_test.sbr
.\X86Dbg\be_all.sbr
.\X86Dbg\cmdline.sbr
.\X86Dbg\config.sbr
.\X86Dbg\dialog.sbr
.\X86Dbg\ldisc.sbr
.\X86Dbg\ldiscucs.sbr
.\X86Dbg\logging.sbr
.\X86Dbg\misc.sbr
.\X86Dbg\noise.sbr
.\X86Dbg\pageantc.sbr
.\X86Dbg\portfwd.sbr
.\X86Dbg\porting.sbr
.\X86Dbg\pproxy.sbr
.\X86Dbg\proxy.sbr
.\X86Dbg\raw.sbr
.\X86Dbg\rlogin.sbr
.\X86Dbg\settings.sbr
.\X86Dbg\sizetip.sbr
.\X86Dbg\ssh.sbr
.\X86Dbg\sshaes.sbr
.\X86Dbg\sshblowf.sbr
.\X86Dbg\sshbn.sbr
.\X86Dbg\sshcrc.sbr
.\X86Dbg\sshcrcda.sbr
.\X86Dbg\sshdes.sbr
.\X86Dbg\sshdh.sbr
.\X86Dbg\sshdss.sbr
.\X86Dbg\sshmd5.sbr
.\X86Dbg\sshpubk.sbr
.\X86Dbg\sshrand.sbr
.\X86Dbg\sshrsa.sbr
.\X86Dbg\sshsh512.sbr
.\X86Dbg\sshsha.sbr
.\X86Dbg\sshzlib.sbr
.\X86Dbg\telnet.sbr
.\X86Dbg\terminal.sbr
.\X86Dbg\tree234.sbr
.\X86Dbg\unicode.sbr
.\X86Dbg\version.sbr
.\X86Dbg\wcemisc.sbr
.\X86Dbg\wcestuff.sbr
.\X86Dbg\wcetime.sbr
.\X86Dbg\wcwidth.sbr
.\X86Dbg\wildcard.sbr
.\X86Dbg\wincfg.sbr
.\X86Dbg\winctrls.sbr
.\X86Dbg\windefs.sbr
.\X86Dbg\windlg.sbr
.\X86Dbg\window.sbr
.\X86Dbg\winmisc.sbr
.\X86Dbg\winnet.sbr
.\X86Dbg\winstore.sbr
.\X86Dbg\winutils.sbr
.\X86Dbg\x11fwd.sbr]
Creating command line "bscmake.exe @C:\DOCUME~1\Aleq\LOCALS~1\Temp\RSP5AF.tmp"
Creating browse info file...
<h3>Output Window</h3>




<h3>Results</h3>
PocketPuTTY2000.exe - 0 error(s), 0 warning(s)
</pre>
</body>
</html>
