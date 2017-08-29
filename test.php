<?php

$ht = new linger\Hashtable();

for ($i = 0; $i < 100; $i++) {
    $ht[$i] = $i;
}

foreach ($ht as $key => $val) {
    echo $key . "::" . $val . "\n";
}
