<?php BoxTop($develNewsStr); ?>

<p>
Оваа веб страна треба да прерасне во место каде ќе може да видите
Што секој од развивачите работи.  Во меѓувреме, тука се
битните информации за Audacity 1.1, која е всушност
сегашната бета верзија.
</p>

<p>
<b>Нови можности во верзијата 1.1:</b>
<pre>
  * Аудио обработка:
    - Подршка за 24 и 32-битни формати
    - Автоматско реалновременско прератирање (користејќи линеарна
        интерполација)
  * Ефекти:
    - Поддршка за LADSPA додавките на Linux / Unix
  * Датотечни формати:
    - Нов XML-базиран Audacity проектен формат
    - Целосна Ogg Vorbis подршка (внес и експорт)
    - Експорт во кој бил окоманднолиниски програм под Unix
    - Подршка за вчитување и запишување многу типови
        неспакувани формати вклучувајќи го и ADPCM WAV форматот.
  * Алатки
    - Нов код за исцртување на алатките; автоматски ги презема
        боите од вашиот оперативен систем
    - Нови копчиња (Скок на почеток, Скок на крај)
    - Ново во уреди алатка
    - Копчињата се исклучени кога се неможни
  * Кориснички интерфејс
    - Целосно менливи кратенки
    - Автоврт при свирење и снимање
    - Нов лењир, употребен во главниот прозорец и во
        FFT Филтер ефектот
    - Брановидната форма сега ги прикажува средните вредности со посветла
        боја отколку врвовите
  * Локализација
    - Audacity сега може да биде локализиран на многу странски
      јазици.
</pre>
</p>

<p>
<b>Табела на употребени библиотеки (верзија 1.1.0 и пред):</b>
<table border=0 cellpadding=8 cellspacing=2>
<tr>
<th>Библиотека</th>
<th>Намена</th>
</tr>
<tr>
<td bgcolor="#ccccff"
><a href="http://www.wxwindows.org">wxWindows</a>
<td bgcolor="#ccccff"
>Разноплатформска библиотека која дозволува нашиот GUI (менија, копчиња,
    прозорци, цртежи и сл.) да работи природно на Mac, Windows и Unix
    системи.  wxWindows нуди и други корисни C++ класи и
    Audacity е 100% базиран на овие библиотеки.  Најсрдечно го препорачуваме
    користењето на овие библиотеки при разноплатформски развој.
</tr>

<tr>
<td bgcolor="#ccccff"
><a href="http://www.mars.org/home/rob/proj/mpeg/">libmad</a>
<td bgcolor="#ccccff"
><p>
    MAD понуда на MPEG аудио декодер.  Ова е еден од
    неколкуте MP3 декодери кои се слободни (GPL) и единствен кој ние
    го одбравме заради математиката и способноста за
    продуцирање 24-битен излез (што уште не сме го искористиле).
    Дури и при MP3 датотеки нормално креирани со 16-битен влез,
    имањето MP3 декодер кој произведува 24-битен излез ни дозволува да користиме
    сопствен делач, да го подигнеме аудио квалитетот
    или бар за подобра корисничка контрола.  Библиотекава е многу брза
    (дури и во споредба со xaudio, која ја користевме порано) и
    многу стабилна.  Напишана е од Rob Leslie.</p>
    <p>Библиотекава е потребна само ако сакате да внесувате MP3</p>
</tr>
<tr>
<td bgcolor="#ccccff"
><a href="http://www.mars.org/home/rob/proj/mpeg/">libid3tag</a></td>
<td bgcolor="#ccccff"
><p>
    И оваа библиотека е напишана од Rob Leslie (авторот на libmad)
	и е убава, едноставна библиотека за читање и пишување ID3 тагови
    во MP3 датотеки.</p>
    <p>Библиотекава е опциска; ако е поврзана корисникот ќе
    види Таг дијалог при експорт на MP3 датотеки и
    и таговите ќе бидат внесени при внес на MP3 датотека.
    Забележете дека libid3tag не се дистрибуира одвоено; таа е
    вклучена како дел MAD.</p>
</td>
</tr>
<tr>
<td bgcolor="#ccccff"
><a href="http://www.xiph.org/ogg/vorbis/"
>libogg<br>libvorbis<br>libvorbisfile</a>
</td>

<td bgcolor="#ccccff"
>
<p>Ogg Vorbis е слободен аудио пакувачки формат заедно со библиотека
која енкодира и декодира датотеки во овој формат.  Тој има намера
да биде замена за MP3 и многумина почивствувале дека тој е ист а
и го надминува MP3 форматот и во квалитет и во големина.
Последната (бета) верзија на Audacity и внесува и експортира
Ogg Vorbis датотеки.
</p>
</td>
</tr>
<tr>
<td bgcolor="#ccccff"
>
<a href="http://www.portaudio.com">portaudio</a>
</td>
<td bgcolor="#ccccff"
>
Оваа библиотека ни дозволува правење аудио В/И на различни платформи
користејќи API.  Таа ги скрива разликите меѓу аудио В/И применети
на различни платформи и глобално нуди многу добри
карактеристики.  Аудио кодот е стабилен за Windows (MME и DirectX),
Unix/OSS и MacOS 9, а преведувањето за MacOS X, Linux/ALSA и Linux/aRts
е во развој.
</td>
</tr>
<tr>
<td bgcolor="#ccccff"
><a href="http://www.zip.com.au/~erikd/libsndfile/">libsndfile</a></td>

<td bgcolor="#ccccff"
>
Ова е нова библиотека која ја користиме за читање и пишување аудио датотеки
како WAV, AIFF и AU.  Користи едноставна компресија како ADPCM,
но не и губливо-пакувачки записи.
</td>
</tr>
</table>
</p>


<?php BoxBottom(); ?>
