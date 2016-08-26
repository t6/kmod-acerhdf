<div class="mandoc">
<table summary="Document Header" class="head" width="100%">
<col width="30%">
<col width="30%">
<col width="30%">
<tbody>
<tr>
<td class="head-ltitle">
ACERHDF(4)</td>
<td class="head-vol" align="center">
FreeBSD Kernel Interfaces Manual</td>
<td class="head-rtitle" align="right">
ACERHDF(4)</td>
</tr>
</tbody>
</table>
<div class="section">
<h1 id="x4e414d45">NAME</h1> <b class="name">acerhdf</b> &#8212; <span class="desc">Acer Aspire One fan control</span></div>
<div class="section">
<h1 id="x4445534352495054494f4e">DESCRIPTION</h1> The <b class="name">acerhdf</b> driver allows you to control the fans of some of the Acer Aspire One netbook models, so that they are not constantly running.  Other netbooks might be supported as well.  See <i class="link-sec"><a class="link-sec" href="#x535550504f525445442044455649434553">SUPPORTED DEVICES</a></i> for more details.  It is a port of the Linux kernel module with the same name.<p>
<b class="name">acerhdf</b> monitors the system temperature and turns the fan on if it is above the fan-on threshold, and turns it off again if the temperature drops below the fan-off threshold.</div>
<div class="section">
<h1 id="x53595343544c205641524941424c4553">SYSCTL VARIABLES</h1><dl style="margin-top: 0.00em;margin-bottom: 0.00em;" class="list list-tag">
<dt class="list-tag" style="margin-top: 1.00em;">
<b class="var">dev.acerhdf.0.enabled</b></dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
Set to 1 if <b class="name">acerhdf</b> should start controlling the fan. Defaults to 0, which means <b class="name">acerhdf</b> is not in control of the fan.</dd>
<dt class="list-tag" style="margin-top: 1.00em;">
<b class="var">dev.acerhdf.0.fanon</b></dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
The temperature at which the fan should be turned on. Defaults to 60.</dd>
<dt class="list-tag" style="margin-top: 1.00em;">
<b class="var">dev.acerhdf.0.fanoff</b></dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
The temperature at which the fan should be turned off again. Defaults to 53.</dd>
<dt class="list-tag" style="margin-top: 1.00em;">
<b class="var">dev.acerhdf.0.fanstate</b></dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
Read-only.  Returns the current fan state, <b class="var">auto</b> if the fan is running or <b class="var">off</b>.</dd>
<dt class="list-tag" style="margin-top: 1.00em;">
<b class="var">dev.acerhdf.0.interval</b></dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
Seconds to wait between temperature polls.  Defaults to 5 seconds.</dd>
<dt class="list-tag" style="margin-top: 1.00em;">
<b class="var">dev.acerhdf.0.temperature</b></dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
Read-only.  The current system temperature in degree Celsius.</dd>
</dl>
</div>
<div class="section">
<h1 id="x535550504f525445442044455649434553">SUPPORTED DEVICES</h1> <b class="name">acerhdf</b> was tested on an Acer Aspire One A150 running FreeBSD 12.0 only.  The FreeBSD port will most likely support the same range of devices as the original Linux version.  This is entirely untested however.<p>
<dl style="margin-top: 0.00em;margin-bottom: 0.00em;margin-left: 5.00ex;" class="list list-tag">
<dt class="list-tag" style="margin-top: 0.00em;">
Acer AO521</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer AO531h</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer AO751h</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Aspire 1410</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Aspire 1810T</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Aspire 1810TZ</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Aspire 1825PTZ</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Aspire 5315</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Aspire 5739G</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Aspire 5755G</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Aspire One 753</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Aspire One A110</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Aspire One A150</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer Extensa 5420</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer LT-10Q</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer TM8573T</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Acer TravelMate 7730G</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Gateway AOA110</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Gateway AOA150</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Gateway LT31</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Packard Bell AOA110</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Packard Bell AOA150</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Packard Bell DOA150</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Packard Bell DOTMA</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Packard Bell DOTMU</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Packard Bell DOTVR46</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
<dt class="list-tag" style="margin-top: 0.00em;">
Packard Bell ENBFT</dt>
<dd class="list-tag" style="margin-left: 6.00ex;">
</dd>
</dl>
</div>
<div class="section">
<h1 id="x4558414d504c4553">EXAMPLES</h1> To enable <b class="name">acerhdf</b> make sure it is installed in <i class="file">/boot/modules</i> and add the following settings to your <i class="file">/boot/loader.conf</i>:<p>
<pre style="margin-left: 5.00ex;" class="lit display">
acerhdf_load="YES"</pre>
<p>
and your <i class="file">/etc/sysctl.conf</i>:<p>
<pre style="margin-left: 5.00ex;" class="lit display">
dev.acerhdf.0.enabled=1</pre>
<p>
You may want to adjust the fan-on and fan-off thresholds as well.</div>
<div class="section">
<h1 id="x42554753">BUGS</h1> The system needs to have a hard drive installed, otherwise <b class="name">acerhdf</b> will not work correctly and the system will automatically power down after some time.  This behavior is enforced by the BIOS and there is no known workaround.  This bug is present in the Linux version as well.</div>
<div class="section">
<h1 id="x53454520414c534f">SEE ALSO</h1> <a class="link-man" href="https://www.freebsd.org/cgi/man.cgi?query=kenv&amp;sektion=1&amp;apropos=0&amp;manpath=FreeBSD+10.2-RELEASE">kenv(1)</a>, <a class="link-man" href="https://www.freebsd.org/cgi/man.cgi?query=loader.conf&amp;sektion=5&amp;apropos=0&amp;manpath=FreeBSD+10.2-RELEASE">loader.conf(5)</a>, <a class="link-man" href="https://www.freebsd.org/cgi/man.cgi?query=sysctl.conf&amp;sektion=5&amp;apropos=0&amp;manpath=FreeBSD+10.2-RELEASE">sysctl.conf(5)</a></div>
<div class="section">
<h1 id="x415554484f5253">AUTHORS</h1> <span class="author"></span><b class="name">acerhdf</b> was ported from Linux to FreeBSD by <span class="author">Tobias Kortkamp</span> &#60;<a class="link-mail" href="mailto:t@tobik.me">t@tobik.me</a>&#62;.<p>
The original Linux version of this module was written by: <span class="author">Peter Feuerer</span> &#60;<a class="link-mail" href="mailto:peter@pie.net">peter@pie.net</a>&#62; and <span class="author">Borislav Petkov</span> &#60;<a class="link-mail" href="mailto:bp@alien8.de">bp@alien8.de</a>&#62;.</div>
<table summary="Document Footer" class="foot" width="100%">
<col width="50%">
<col width="50%">
<tbody>
<tr>
<td class="foot-date">
August 26, 2016</td>
<td class="foot-os" align="right">
FreeBSD 10.3</td>
</tr>
</tbody>
</table>
</div>

