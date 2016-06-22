/*
 * GPS Navigation ---An open source GPS navigation core software
 *
 *
 * Copyright (c) 2016  Hitachi, Ltd.
 *
 * This program is dual licensed under GPL version 2 or a commercial license.
 * See the LICENSE file distributed with this source file.
 */

/*
 * VoiceText.h
 *
 *  Created on: 2014/11/12
 *      Author: masutani
 */

#ifndef VOICETEXT_H_
#define VOICETEXT_H_

#define VOICE_TEXT_301 301     //%d
#define VOICE_TEXT_302 302     //%s
#define VOICE_TEXT_303 303     //、
#define VOICE_TEXT_304 304     //。
#define VOICE_TEXT_305 305     //,
#define VOICE_TEXT_306 306     //.

#define VOICE_TEXT_JP_101 1101 //実際の交通規制に従って走行して下さい。
#define VOICE_TEXT_JP_102 1102 //ラウンドアバウトがあります
#define VOICE_TEXT_JP_103 1103 //目的地へのルートガイドを開始します。
#define VOICE_TEXT_JP_104 1104 //まもなく、目的地付近です。ルートガイドを終了します。
#define VOICE_TEXT_JP_105 1105 //この先、経路が切れています。実際の交通規制に従って走行して下さい。
#define VOICE_TEXT_JP_106 1106 //この先、暫く道なりです。

#define VOICE_TEXT_JP_201 1201 //1
#define VOICE_TEXT_JP_202 1202 //2
#define VOICE_TEXT_JP_203 1203 //3
#define VOICE_TEXT_JP_204 1204 //4
#define VOICE_TEXT_JP_205 1205 //5
#define VOICE_TEXT_JP_206 1206 //1番目
#define VOICE_TEXT_JP_207 1207 //2番目
#define VOICE_TEXT_JP_208 1208 //3番目
#define VOICE_TEXT_JP_209 1209 //4番目
#define VOICE_TEXT_JP_210 1210 //5番目
#define VOICE_TEXT_JP_211 1211 //6番目
#define VOICE_TEXT_JP_212 1212 //7番目
#define VOICE_TEXT_JP_213 1213 //8番目
#define VOICE_TEXT_JP_214 1214 //9番目
#define VOICE_TEXT_JP_215 1215 //10番目
#define VOICE_TEXT_JP_216 1216 //11番目
#define VOICE_TEXT_JP_217 1217 //12番目
#define VOICE_TEXT_JP_218 1218 //13番目
#define VOICE_TEXT_JP_219 1219 //14番目
#define VOICE_TEXT_JP_220 1220 //15番目
#define VOICE_TEXT_JP_221 1221 //16番目

#define VOICE_TEXT_JP_401 1401 //直進方向
#define VOICE_TEXT_JP_402 1402 //Ｕターン
#define VOICE_TEXT_JP_403 1403 //斜め右方向
#define VOICE_TEXT_JP_404 1404 //右方向
#define VOICE_TEXT_JP_405 1405 //大きく右方向
#define VOICE_TEXT_JP_406 1406 //斜め左方向
#define VOICE_TEXT_JP_407 1407 //左方向
#define VOICE_TEXT_JP_408 1408 //大きく左方向
#define VOICE_TEXT_JP_409 1409 //右方向
#define VOICE_TEXT_JP_410 1410 //左方向
#define VOICE_TEXT_JP_411 1411 //右方向
#define VOICE_TEXT_JP_412 1412 //左方向
#define VOICE_TEXT_JP_413 1413 //目的地
#define VOICE_TEXT_JP_414 1414 //経由地
#define VOICE_TEXT_JP_415 1415 //300m
#define VOICE_TEXT_JP_416 1416 //100m
#define VOICE_TEXT_JP_417 1417 //交差点
#define VOICE_TEXT_JP_418 1418 //分岐
#define VOICE_TEXT_JP_419 1419 //1km
#define VOICE_TEXT_JP_420 1420 //500m

#define VOICE_TEXT_JP_501 1501 //まもなく
#define VOICE_TEXT_JP_502 1502 //およそ
#define VOICE_TEXT_JP_503 1503 //この先
#define VOICE_TEXT_JP_504 1504 //その先
#define VOICE_TEXT_JP_505 1505 //先
#define VOICE_TEXT_JP_506 1506 //を
#define VOICE_TEXT_JP_507 1507 //分岐を
#define VOICE_TEXT_JP_508 1508 //交差点を
#define VOICE_TEXT_JP_509 1509 //出口は手前から
#define VOICE_TEXT_JP_510 1510 //です。
#define VOICE_TEXT_JP_511 1511 //付近です。
#define VOICE_TEXT_JP_512 1512 //高速道路入口です。
#define VOICE_TEXT_JP_513 1513 //高速道路出口です。
#define VOICE_TEXT_JP_514 1514 //へのルートガイドを開始します。
#define VOICE_TEXT_JP_515 1515 //暫く道なりです。

#define VOICE_TEXT_EN_101 2101 //Follow actual traffic regulations.
#define VOICE_TEXT_EN_102 2102 //There is a roundabout.
#define VOICE_TEXT_EN_103 2103 //Starting route guidance to the destination.
#define VOICE_TEXT_EN_104 2104 //You have reached your destination. Ending route guidance.
#define VOICE_TEXT_EN_105 2105 //You have reached your destination.
#define VOICE_TEXT_EN_106 2106 //No route guidance from here. Follow actual traffic regulations.
#define VOICE_TEXT_EN_107 2107 //Continue on the current route.

#define VOICE_TEXT_EN_201 2201 //one
#define VOICE_TEXT_EN_202 2202 //two
#define VOICE_TEXT_EN_203 2203 //three
#define VOICE_TEXT_EN_204 2204 //four
#define VOICE_TEXT_EN_205 2205 //five
#define VOICE_TEXT_EN_206 2206 //first
#define VOICE_TEXT_EN_207 2207 //second
#define VOICE_TEXT_EN_208 2208 //third
#define VOICE_TEXT_EN_209 2209 //fourth
#define VOICE_TEXT_EN_210 2210 //fifth
#define VOICE_TEXT_EN_211 2211 //sixth
#define VOICE_TEXT_EN_212 2212 //seventh
#define VOICE_TEXT_EN_213 2213 //eight
#define VOICE_TEXT_EN_214 2214 //9 th
#define VOICE_TEXT_EN_215 2215 //10 th
#define VOICE_TEXT_EN_216 2216 //11 th
#define VOICE_TEXT_EN_217 2217 //12 th
#define VOICE_TEXT_EN_218 2218 //13 th
#define VOICE_TEXT_EN_219 2219 //14 th
#define VOICE_TEXT_EN_220 2220 //15 th
#define VOICE_TEXT_EN_221 2221 //16 th

#define VOICE_TEXT_EN_401 2401 //go straight
#define VOICE_TEXT_EN_402 2402 //make a u-turn
#define VOICE_TEXT_EN_403 2403 //slight right turn
#define VOICE_TEXT_EN_404 2404 //right turn
#define VOICE_TEXT_EN_405 2405 //sharp right turn
#define VOICE_TEXT_EN_406 2406 //slight left turn
#define VOICE_TEXT_EN_407 2407 //left turn
#define VOICE_TEXT_EN_408 2408 //sharp left turn
#define VOICE_TEXT_EN_409 2409 //keep right
#define VOICE_TEXT_EN_410 2410 //keep left
#define VOICE_TEXT_EN_411 2411 //on your right
#define VOICE_TEXT_EN_412 2412 //on your left
#define VOICE_TEXT_EN_413 2413 //destination
#define VOICE_TEXT_EN_414 2414 //waypoint
#define VOICE_TEXT_EN_415 2415 //one half mile
#define VOICE_TEXT_EN_416 2416 //one quarter mile

#define VOICE_TEXT_EN_501 2501 //Starting route guidance to the
#define VOICE_TEXT_EN_502 2502 //You have reached your waypoint
#define VOICE_TEXT_EN_503 2503 //Highway entrance
#define VOICE_TEXT_EN_504 2504 //Highway exit
#define VOICE_TEXT_EN_505 2505 //In about
#define VOICE_TEXT_EN_506 2506 //ahead.
#define VOICE_TEXT_EN_507 2507 //there is a roundabout
#define VOICE_TEXT_EN_508 2508 //take the
#define VOICE_TEXT_EN_509 2509 //then
#define VOICE_TEXT_EN_510 2510 //exit
#define VOICE_TEXT_EN_511 2511 //Is the

#define VOICE_TEXT2_301 "%d"
#define VOICE_TEXT2_302 "%s"
#define VOICE_TEXT2_303 "、"
#define VOICE_TEXT2_304 "。"
#define VOICE_TEXT2_305 ","
#define VOICE_TEXT2_306 "."

#define VOICE_TEXT2_JP_101 "実際の交通規制に従って走行して下さい。"
#define VOICE_TEXT2_JP_102 "ラウンドアバウトがあります。"
#define VOICE_TEXT2_JP_103 "目的地へのルートガイドを開始します。"
#define VOICE_TEXT2_JP_104 "まもなく、目的地付近です。ルートガイドを終了します。"
#define VOICE_TEXT2_JP_105 "この先、経路が切れています。実際の交通規制に従って走行して下さい。"
#define VOICE_TEXT2_JP_106 "この先、暫く道なりです。"

#define VOICE_TEXT2_JP_201 "1"
#define VOICE_TEXT2_JP_202 "2"
#define VOICE_TEXT2_JP_203 "3"
#define VOICE_TEXT2_JP_204 "4"
#define VOICE_TEXT2_JP_205 "5"
#define VOICE_TEXT2_JP_206 "1番目"
#define VOICE_TEXT2_JP_207 "2番目"
#define VOICE_TEXT2_JP_208 "3番目"
#define VOICE_TEXT2_JP_209 "4番目"
#define VOICE_TEXT2_JP_210 "5番目"
#define VOICE_TEXT2_JP_211 "6番目"
#define VOICE_TEXT2_JP_212 "7番目"
#define VOICE_TEXT2_JP_213 "8番目"
#define VOICE_TEXT2_JP_214 "9番目"
#define VOICE_TEXT2_JP_215 "10番目"
#define VOICE_TEXT2_JP_216 "11番目"
#define VOICE_TEXT2_JP_217 "12番目"
#define VOICE_TEXT2_JP_218 "13番目"
#define VOICE_TEXT2_JP_219 "14番目"
#define VOICE_TEXT2_JP_220 "15番目"
#define VOICE_TEXT2_JP_221 "16番目"

#define VOICE_TEXT2_JP_401 "直進方向"
#define VOICE_TEXT2_JP_402 "Ｕターン"
#define VOICE_TEXT2_JP_403 "斜め右方向"
#define VOICE_TEXT2_JP_404 "右方向"
#define VOICE_TEXT2_JP_405 "大きく右方向"
#define VOICE_TEXT2_JP_406 "斜め左方向"
#define VOICE_TEXT2_JP_407 "左方向"
#define VOICE_TEXT2_JP_408 "大きく左方向"
#define VOICE_TEXT2_JP_409 "右方向"
#define VOICE_TEXT2_JP_410 "左方向"
#define VOICE_TEXT2_JP_411 "右方向"
#define VOICE_TEXT2_JP_412 "左方向"
#define VOICE_TEXT2_JP_413 "目的地"
#define VOICE_TEXT2_JP_414 "経由地"
#define VOICE_TEXT2_JP_415 "300m"
#define VOICE_TEXT2_JP_416 "100m"
#define VOICE_TEXT2_JP_417 "交差点"
#define VOICE_TEXT2_JP_418 "分岐"
#define VOICE_TEXT2_JP_419 "1kmm"
#define VOICE_TEXT2_JP_420 "500m"

#define VOICE_TEXT2_JP_501 "まもなく"
#define VOICE_TEXT2_JP_502 "およそ"
#define VOICE_TEXT2_JP_503 "この先"
#define VOICE_TEXT2_JP_504 "その先"
#define VOICE_TEXT2_JP_505 "先"
#define VOICE_TEXT2_JP_506 "を"
#define VOICE_TEXT2_JP_507 "分岐を"
#define VOICE_TEXT2_JP_508 "交差点を"
#define VOICE_TEXT2_JP_509 "出口は手前から"
#define VOICE_TEXT2_JP_510 "です。"
#define VOICE_TEXT2_JP_511 "付近です。"
#define VOICE_TEXT2_JP_512 "高速道路入口です。"
#define VOICE_TEXT2_JP_513 "高速道路出口です。"
#define VOICE_TEXT2_JP_514 "へのルートガイドを開始します。"
#define VOICE_TEXT2_JP_515 "暫く道なりです。"

#define VOICE_TEXT2_EN_101 " Follow actual traffic regulations."
#define VOICE_TEXT2_EN_102 " There is a roundabout."
#define VOICE_TEXT2_EN_103 " Starting route guidance to the destination."
#define VOICE_TEXT2_EN_104 " You have reached your destination. Ending route guidance."
#define VOICE_TEXT2_EN_105 " You have reached your destination."
#define VOICE_TEXT2_EN_106 " No route guidance from here. Follow actual traffic regulations."
#define VOICE_TEXT2_EN_107 " Continue on the current route."

#define VOICE_TEXT2_EN_201 " one"
#define VOICE_TEXT2_EN_202 " two"
#define VOICE_TEXT2_EN_203 " three"
#define VOICE_TEXT2_EN_204 " four"
#define VOICE_TEXT2_EN_205 " five"
#define VOICE_TEXT2_EN_206 "  first"
#define VOICE_TEXT2_EN_207 " second"
#define VOICE_TEXT2_EN_208 " third"
#define VOICE_TEXT2_EN_209 " fourth"
#define VOICE_TEXT2_EN_210 " fifth"
#define VOICE_TEXT2_EN_211 " sixth"
#define VOICE_TEXT2_EN_212 " seventh"
#define VOICE_TEXT2_EN_213 " eight"
#define VOICE_TEXT2_EN_214 " 9 th"
#define VOICE_TEXT2_EN_215 " 10 th"
#define VOICE_TEXT2_EN_216 " 11 th"
#define VOICE_TEXT2_EN_217 " 12 th"
#define VOICE_TEXT2_EN_218 " 13 th"
#define VOICE_TEXT2_EN_219 " 14 th"
#define VOICE_TEXT2_EN_220 " 15 th"
#define VOICE_TEXT2_EN_221 " 16 th"

#define VOICE_TEXT2_EN_401 " go straight"
#define VOICE_TEXT2_EN_402 " make a u-turn"
#define VOICE_TEXT2_EN_403 " slight right turn"
#define VOICE_TEXT2_EN_404 " right turn"
#define VOICE_TEXT2_EN_405 " sharp right turn"
#define VOICE_TEXT2_EN_406 " slight left turn"
#define VOICE_TEXT2_EN_407 " left turn"
#define VOICE_TEXT2_EN_408 " sharp left turn"
#define VOICE_TEXT2_EN_409 " keep right"
#define VOICE_TEXT2_EN_410 " keep left"
#define VOICE_TEXT2_EN_411 " on your right"
#define VOICE_TEXT2_EN_412 " on your left"
#define VOICE_TEXT2_EN_413 " destination"
#define VOICE_TEXT2_EN_414 " waypoint"
#define VOICE_TEXT2_EN_415 " one half mile"
#define VOICE_TEXT2_EN_416 " one quarter mile"

#define VOICE_TEXT2_EN_501 " Starting route guidance to the"
#define VOICE_TEXT2_EN_502 " You have reached your waypoint"
#define VOICE_TEXT2_EN_503 " Highway entrance"
#define VOICE_TEXT2_EN_504 " Highway exit"
#define VOICE_TEXT2_EN_505 " In about"
#define VOICE_TEXT2_EN_506 " ahead."
#define VOICE_TEXT2_EN_507 " there is a roundabout"
#define VOICE_TEXT2_EN_508 " take the"
#define VOICE_TEXT2_EN_509 " then"
#define VOICE_TEXT2_EN_510 " exit"
#define VOICE_TEXT2_EN_511 " Is the"

#endif /* VOICETEXT_H_ */
