<?

# perform a redirect to the latest version, so links to it can remain static

// logging
$d = date("Y-m-d G:i:s");
$fp = fopen("download_stats.txt", "a");
fwrite($fp, "$d\ti386-rpm\n");
fclose($fp);

include 'mirror.php.inc';
include 'versions.php.inc';
header('Location: '.mirror.'audacity-'.i386rpm.'.i386.rpm');

?>
