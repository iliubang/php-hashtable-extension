<?php

$ht = new Linger\Hashtable(65535);
var_dump($ht);

$ht->set("hello", "world");
$ht->set("name", "liubang");
$ht->foreach(function($key, $val) {
    echo $key, PHP_EOL;
    var_dump($val);
});
die;
var_dump($ht->getCount());
$ht->del("hello");
var_dump($ht->getCount());

$ht->set("hello8800", "world");
echo $ht->get("hello8800")."\n";
var_dump($ht->del("hello8800"));

var_dump($ht->get("liubang"));

