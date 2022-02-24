<?php
$directory = dirname (__FILE__);
$contents = scandir ($directory);

$recent_file = null;
$modified = 0;
foreach ($contents as $filename)
{
    if (is_file ($filename))
    {
    	$path_parts = pathinfo ($filename);
        if (strcmp ($path_parts['extension'], 'bin') == 0 && filemtime ($filename) > $modified)
        {
          $modified = filemtime ($filename);
          $recent_file = $filename;
        }
    }
}

$size = filesize($recent_file);
header('Content-Type: application/octet-stream');
header('Content-Disposition: attachment; filename="'.$recent_file.'"');
header("Content-length: $size");
readfile($recent_file);

?>