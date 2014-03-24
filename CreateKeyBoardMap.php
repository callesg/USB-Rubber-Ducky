<?php
/*
First Run this to generate a linux keyboard map dump
sudo dumpkeys -c iso-8859-1 -1 -n --keys-only > /tmp/dumpkey.map
*/
$DefaultEncoding = "iso-8859-1";
mb_internal_encoding("iso-8859-1"); 

$dbg = false;
$dbg = true;

//Characters that map to other charaters
$SpecialMapping = array(
	//10 => 13 //LineFeed maps to Carriage Return
	//13 => 10 //Carriage Return maps to LineFeed
);
$Ignore = array(
	13
);



//Source of this table the linux kernel drivers/usb/hid-input.c
$HidCodes2Linux = array(
	0,  0,  0,  0, 30, 48, 46, 32, 18, 33, 34, 35, 23, 36, 37, 38,
	50, 49, 24, 25, 16, 19, 31, 20, 22, 47, 17, 45, 21, 44,  2,  3,
	4,  5,  6,  7,  8,  9, 10, 11, 28,  1, 14, 15, 57, 12, 13, 26,
	27, 43, 84, 39, 40, 41, 51, 52, 53, 58, 59, 60, 61, 62, 63, 64,
	65, 66, 67, 68, 87, 88, 99, 70,119,110,102,104,111,107,109,106,
	105,108,103, 69, 98, 55, 74, 78, 96, 79, 80, 81, 75, 76, 77, 71,
	72, 73, 82, 83, 86,127,116,117, 85, 89, 90, 91, 92, 93, 94, 95,
	120,121,122,123,134,138,130,132,128,129,131,137,133,135,136,113,
	115,114,NULL,NULL,NULL,124,NULL,181,182,183,184,185,186,187,188,189,
	190,191,192,193,194,195,196,197,198,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
	29, 42, 56,125, 97, 54,100,126,164,166,165,163,161,115,114,113,
	150,158,159,128,136,177,178,176,142,152,173,140,NULL,NULL,NULL,NULL
);

function unichr($u) {
	$DecVal = hexdec($u);
    return mb_convert_encoding('&#' . $DecVal . ';', mb_internal_encoding(), 'HTML-ENTITIES');
}

function unichrf($u) {
	$DecVal = hexdec($u);
    return mb_convert_encoding(pack("N",$DecVal), mb_internal_encoding(), 'UTF-8');
}
function iso8859chr($u) {
	global $DefaultEncoding;
	$DecVal = hexdec($u);
    return mb_convert_encoding(pack("N",$DecVal), mb_internal_encoding(), $DefaultEncoding);
}


$Charcters = array();

if (($handle = fopen("/tmp/dumpkey.map", "r")) !== FALSE) {

    while (($line = fgetcsv($handle, 0, "=")) !== FALSE) {

        $collumnCount = count($line);
		//Ignore unimportant lines
		if($collumnCount > 1){
			$CharacterOrg = trim(array_pop($line));
			$CharType = substr($CharacterOrg, 0, 1);
			switch($CharType){
				case '0':
					//All chars seams to start with 1 byte that has some sort of meaning but we skip that byte
					$Character = hexdec(substr($CharacterOrg, 4));//iso8859chr(substr($CharacterOrg, 4));
					break;
				case 'U'://Is a unicode Char
					$Character = ord(unichr(substr($CharacterOrg, 2)));
					break;
				case '+'://is Shiftable Shitable stuff seaams to allwas start with 0b
					$Character = hexdec(substr($CharacterOrg, 5));//iso8859chr(substr($CharacterOrg, 5));
					break;
				default:
					throw new Exception("Unknown Character encoding on keystrike: ".$line[0]);
			}
			//hexdec(bin2hex($CharValue))
			//$Character = utf8_decode($Character);
			//if(count($Character) > 1){
			// continue;//Ignore UTF8 Chars that is longer than one byte
			//}
			
			if(!isset($Charcters[$Character])){
				$Charcters[$Character] = explode('	', $line[0]);
				array_shift($Charcters[$Character]);
				$LinuxCode = intval(array_pop(explode(" ", (trim(array_pop($Charcters[$Character]))))));
				$Modifiers = array();
				foreach($Charcters[$Character] AS $ModifierKey){
					$Modifiers[] = $ModifierKey;
				}
				$Charcters[$Character] = array(
					'Modifiers' => $Modifiers,
					'LinuxCode' => $LinuxCode,
				);
			}
		}
    }
    fclose($handle);
}else{
	throw new Exception("Could not read the keydump file /tmp/dumpkey.map");
}

//overwrite enter \n
$Charcters[10] = array(
	'Modifiers' => array(),
	'LinuxCode' => 28,
);

foreach($Ignore AS $CharIgnore){
	if(isset($Charcters[$CharIgnore])){
		unset($Charcters[$CharIgnore]);
	}
}


$HidModifierValues = array(
	'control' => 1,//left controll as standard contorl
	'ctrll' => 1,
	'shift' => 2,//left shift as standard shift
	'shiftl' => 2,
	'alt' => 4,//left allt
	'lgui' => 8,
	'ctrlr' => 16,
	'shiftr' => 32,
	'altgr' => 64,
	'rgui' => 128,
);
$LinuxModifierValues = array(
	'shift' => 1,
	'altgr' => 2,
	'control' => 4,
	'alt' => 8,
	'shiftl' => 16,
	'shiftr' => 32,
	'ctrll' => 64,
	'ctrlr' => 128,
	'capsshift' => 256,
);

ksort($Charcters);
foreach($Charcters AS $CharValue => $Character){
	$FromChar = $CharValue;
	if(isset($SpecialMapping[$CharValue])){
		$FromChar = $SpecialMapping[$CharValue];
	}
	$LinuxCode = $Charcters[$FromChar]['LinuxCode'];
	$HidCode = array_search($LinuxCode, $HidCodes2Linux);
	if($HidCode !== FALSE){
		$ModifierChar = 0;
		foreach($Charcters[$FromChar]['Modifiers'] AS $ModifierKey){
			$ModifierChar |= $HidModifierValues[$ModifierKey];
		}
		if(!$dbg){
			echo(pack("C",$CharValue));
			echo(pack("C", $HidCode));
			echo(pack("C", $ModifierChar));
		}
		$Charcters[$CharValue]['keycode'] = $HidCode;
		$Charcters[$CharValue]['chr'] = pack("C", $CharValue);
	}else{
		//This character is not printable on a usb hid keyboard
		unset($Charcters[$CharValue]);
	}
}
if($dbg){
	var_dump($Charcters);
}

?>
