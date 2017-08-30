<?php

$ht = new linger\Hashtable();
var_dump($ht);

for ($i = 0; $i < 100; $i++) {
    //$ht[$i] = $i;
    $ht->set($i, $i);
}

foreach ($ht as $key => $val) {
    echo $key . "::" . $val . "\n";
}
