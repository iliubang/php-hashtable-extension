<?php

$ht = new linger\Hashtable();
var_dump($ht);

for ($i = 0; $i < 100; $i++) {
    $ht[$i] = $i;
//    $ht->set("hello" . $i, $i);

}
for ($i = 0; $i < 100; $i++) {
//    echo $ht->get($i), "\n";

    echo $ht[$i], "\n";
}

echo count($ht);

echo "===============\n";
foreach ($ht as $key => $val) {
    echo $key . "::" . $val . "\n";
}
