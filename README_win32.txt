================================================
adfchk v0.2.0 by A. Eibach (c) 2007 - 2010               01 Nov 2009
================================================

This file is for WINDOWS USERS ONLY.
(Or LINUX users keen to experiment when using the win32 binary
 in DOSEMU and the like :))
 
 adfchk has a low version number. This is PURPOSE. I am not going to
 brag with high version numbers until a basic GUI is finished.
 And since this rather distracts me from my work on bit level,
 I'm rather inclined to postpone this to a yet later version and 
 instead add link virus detection support before creating a shiny,
 but boring GUI...:)
 
 -----------------------------------------------------------------
 
 Here's a short instruction how to use the enclosed batch files:
 
 ------------
 CHKALL.BAT
 ------------
 
 As the name suggests, this will check *ALL* supported types:
 *.DMS files, *.ZIP files and, if there, also plain *.ADFs.
 
 ------------
 CHKDMS.BAT
 ------------
 
 Will check DMS archives only.
 
 
 ------------
 CHKZIP.BAT
 ------------
 
 Will check ZIP archives only.
 
 
 ------------
 CHKADF.BAT
 ------------
 
 Will check *PLAIN* Amiga Disk Files only.
 Any archived ADFs will be ignored!
 
 ----------------------------------------------------------
 
 HINT FROM THE AUTHOR: =) 
 
 If you like to avoid "carrying" LiteUnzip.dll with you every time
 you want to scan one of your deep trees, you can do the following:
 
 (I do it the same myself when testing, by the way)
 
 - copy CHK???.BAT, LiteUnzip.dll and of course adfchk.exe to C:\ (root)
 (or elsewhere, just trying to give an easy solution with less typing)
 
 - type
 -----------------------
  C:\Users\yourusername> path %PATH%;C:\
  ----------------------
  at the DOS prompt. (The 'textbox' contains your DOS prompt; you might see
                      something different on your machines, of course.)
  
  Now you can scan with adfchk.exe whenever you want and still (!!!) get 
  all the log files in the folders where you expect them to be!
 