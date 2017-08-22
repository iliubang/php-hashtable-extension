<?php

$ht = new Linger\Hashtable(65535);
var_dump($ht);

$ht->set("hello", "world");
$ht->set("name", "liubang");
var_dump($ht->getCount());
$ht->del("hello");
var_dump($ht->getCount());

