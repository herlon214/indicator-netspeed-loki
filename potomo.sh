#!/bin/sh
xgettext -k_ indicator-netspeed-unity.c -o indicator-netspeed-unity.pot
echo >>indicator-netspeed-unity.pot
echo msgid \"All\" >>indicator-netspeed-unity.pot
echo msgstr \"\" >>indicator-netspeed-unity.pot
cp indicator-netspeed-unity.pot ./po/indicator-netspeed-unity.pot
cd po
f=`find -name \*.po`
for po_file in $f
do
echo "Processing ${po_file}"
po_file=`echo ${po_file} | sed -e 's/.\///'`
lang=`echo ${po_file} | sed -e 's/.po$//'`
echo ${po_file}
echo ${lang}
msgmerge -U ${lang}.po indicator-netspeed-unity.pot
if [ "$@" == "" ];
then
mo_dir=usr/share/locale/${lang}/LC_MESSAGES
else
mo_dir=$@/${lang}/LC_MESSAGES
fi
mkdir -p ${mo_dir}
msgfmt ${lang}.po -o ${mo_dir}/indicator-netspeed-unity.mo
done
cd ..
