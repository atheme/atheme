#!/bin/sh
# Copyright (c) 2006-2007 Jilles Tjoelker
# See doc/LICENSE for licensing terms
#
# Script to create simple HTML from the help files

htmldir=${1:-tools/htmlhelp}
helpdir=${2:-help/default}

mkdir -p "$htmldir"

{
echo "<html><head><title>Atheme help</title>"
echo "<meta name=\"generator\" content=\"atheme html_helpfiles.sh\">"
echo "</head><body>"
echo "<h1>Atheme help</h1>"
echo "Services"
echo "<ul>"
for d in $helpdir/*; do
	[ -d "$d" ] || continue
	service="${d##*/}"
	case "$service" in
		alis) service=ALIS ;;
		botserv) service=BotServ ;;
		cservice) service=ChanServ ;;
		gservice) service=Global ;;
		groupserv) service=GroupServ ;;
		helpserv) service=HelpServ ;;
		hostserv) service=HostServ ;;
		memoserv) service=MemoServ ;;
		nickserv) service=NickServ ;;
		oservice) service=OperServ ;;
		userserv) service=UserServ ;;
		gameserv) service=GameServ ;;
		infoserv) service=InfoServ ;;
		*) continue ;;
	esac
	echo "<li><a href=\"$service.html\">$service</a>"
	{
		echo "<html><head><title>Atheme help - $service</title>"
		echo "<meta name=\"generator\" content=\"atheme html_helpfiles.sh\">"
		echo "</head><body>"
		echo "</head><body>"
		echo "<h1>$service</h1>"
		for f in $d/*; do
			[ -f "$f" ] || continue
			sed -e '/^#/d' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' -e "s/&nick&/$service/g" -e 's#^Help for \(.*\).*#<a name="\1"><h2>\1</h2></a>#' -e 's#^Examples*:$#<strong>&</strong>#' -e 's#^Syntax: \(.*\)$#<strong>Syntax:</strong> <tt>\1</tt><br>#' -e 's#\([^]*\)#<b>\1</b>#g' -e 's#\([^]*\)#<u>\1</u>#g' -e 's#^$#<p>#' -e 's#^    \(.*\)$#<br><tt>\1</tt>#' $f
		done
		echo "</body></html>"
	} > "$htmldir/$service.html"
done
echo "</ul>"
echo "<p><i>Generated `LC_ALL=C date`</i>"
echo "</body></html>"
} > "$htmldir/index.html"
