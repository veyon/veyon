ADMX files for Active Directory integration of iTALC have been provided by
Peter Gigengack <Peter/dot/Gigengack/at/sthildas/dot/wa/dot/edu/dot/au>:

================================================================================

I believe I have successfully created an ADMX template that will work for
iTALC 2. Each of the policies when created will control the parameters in use
created/modified when using the iTALC Management Console.
 
The only limitation as mentioned in previous emails is using the Active Directory
integrated or “Logon settings” from authentication where binary values are
created in registry which cannot be created with an ADMX template in the group
policy.  This would be best created using the command line options as suggested
in your last email:
 
  I guess you won't be able to somehow generate this binary data. Easiest 
  possibility will be to export current settings in IMC as XML file, delete all 
  undesired settings from that file (e.g. only leave the LogonACL tags etc.) and 
  run  "imc -ApplySettings mySettings.xml".
 
From the testing in my AD environment the settings do get applied correctly.
I would suggest that a couple people who may be interested could try it if they
were happy to test it. I have filled out the explanations/help sections of each
policy as much as possible within the ADMX template, however there were several
that I left.

================================================================================

