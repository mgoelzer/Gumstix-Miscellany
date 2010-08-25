1.  Need to add two options to CGI dir's configuration:

<Directory "/your/cgi-bin">
    [...]
    Options ExecCGI FollowSymLinks
    [...]
</Directory>

2.  Need to set up script aliases:

<IfModule alias_module>
    [...]
    ScriptAlias /bit "/usr/share/apache2/cgi-bin/random-cgi.py"
    ScriptAlias /seed "/usr/share/apache2/cgi-bin/random-cgi.py"
    ScriptAlias /4 "/usr/share/apache2/cgi-bin/random-cgi.py"
    ScriptAlias /16 "/usr/share/apache2/cgi-bin/random-cgi.py"
    ScriptAlias /4096 "/usr/share/apache2/cgi-bin/random-cgi.py"
    ScriptAlias /statistics "/usr/share/apache2/cgi-bin/statistics.py"
    [...]
</IfModule>

