<?php


$ht = new linger\Hashtable();
var_dump($ht);

for ($i = 0; $i < 100; $i++) {
    $ht[$i] = $i;

}
for ($i = 0; $i < 100; $i++) {
    echo $ht[$i], "\n";
}
echo count($ht), "\n";

$ht->foreach(function($key, $val) {
    echo $key . "::" . $val . "\n";
});

foreach ($ht as $key => $val) {
    echo $key . "::" . $val . "\n";
}
