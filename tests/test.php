<?php

$ht = new linger\Hashtable();
var_dump($ht);


$ht->set("name", "liubang");
var_dump($ht->get("name"));

$n = 100;

for ($i = 0; $i < $n; $i++) {
    
    $ht->set("hello{$i}", "world{$i}");
}
var_dump($ht->getCount());

for ($i = 0; $i < $n; $i++) {
    var_dump($ht->get("hello{$i}"));
}

$ht->foreach(function($key, $val) {
    echo "key: {$key}, val: {$val}\n";
});
