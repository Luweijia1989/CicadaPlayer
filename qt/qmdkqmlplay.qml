/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [1]
import QtQuick 2.0
import QtQuick.Window 2.2
import MDKPlayer 1.0
import SimplePlayer 1.0
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

Item {

    property var hz1 : "F:\\dingtalk\\飞麦&拼装DIY_提交文件\\飞麦&拼装DIY_提交文件\\拼装DIY测试提交\\diy01.mp4"
    property var hz2 : "F:\\dingtalk\\飞麦&拼装DIY_提交文件\\飞麦&拼装DIY_提交文件\\拼装DIY测试提交\\diy02.mp4"
    property var hz3 : "F:\\dingtalk\\飞麦&拼装DIY_提交文件\\飞麦&拼装DIY_提交文件\\拼装DIY测试提交\\diy03.mp4"
    property var hz4 : "F:\\dingtalk\\飞麦&拼装DIY_提交文件\\飞麦&拼装DIY_提交文件\\拼装DIY测试提交\\diy04.mp4"


    property var hlqk1 : "F:\\dingtalk\\飞麦&拼装DIY_提交文件\\飞麦&拼装DIY_提交文件\\拼装DIY测试提交\\crf_base01.mp4"
    property var hlqk2 : "F:\\dingtalk\\飞麦&拼装DIY_提交文件\\飞麦&拼装DIY_提交文件\\拼装DIY测试提交\\crf_gender02.mp4"
    property var hlqk3 : "F:\\dingtalk\\飞麦&拼装DIY_提交文件\\飞麦&拼装DIY_提交文件\\拼装DIY测试提交\\crf_caidai03.mp4"
    property var hlqk4 : "F:\\dingtalk\\飞麦&拼装DIY_提交文件\\飞麦&拼装DIY_提交文件\\拼装DIY测试提交\\crf_huoyan04.mp4"

    property bool mutilEnded: false

    width: 320
    height: 480

    visible: true

//    MDKPlayer {
//        id: player
//        anchors.fill: parent

//        onEnded: {
//            stop()
//        }
//    }

    SimplePlayer {
        id:player1
        visible: !mutilEnded
        sourceTag:"tag1"
        anchors.fill: parent
        onEnded:{
            console.log("onEnded","tag1")
            mutilEnded = true
        }
    }

    SimplePlayer {
        id:player2
        visible: !mutilEnded
        sourceTag:"tag2"
        anchors.fill: parent
        onEnded:{
            console.log("onEnded","tag2")
            mutilEnded = true
        }
    }

    SimplePlayer {
        id:player3
        visible: !mutilEnded
        sourceTag:"tag3"
        anchors.fill: parent
        onEnded:{
            console.log("onEnded","tag3")
            mutilEnded = true
        }
    }

    SimplePlayer {
        id:player4
        visible: !mutilEnded
        sourceTag:"tag4"
        anchors.fill: parent
        onEnded:{
            console.log("onEnded","tag4")
            mutilEnded = true
        }
    }

    ColumnLayout {
        width:player1.width
        height:parent.height
        spacing:10
        Button {
            text: "vapbtn"
            onClicked: {
                player1.play1()
            }
        }

        Button {
            text: "mutilRes"
            onClicked: {
                mutilEnded = false
                player1.play3(hlqk1)
                player2.play3(hlqk2)
                player3.play3(hlqk3)
                player4.play3(hlqk4)
                
            }
        }
        Item {
            Layout.fillHeight:true
        }
    }

//    Rectangle {
//        id: rec
//        width: 100
//        height: 100
//        color: "gray"

//        NumberAnimation {
//            target: rec
//            running: true
//            property: "width"
//            duration: 2000
//            from: 100
//            to: 0
//            loops: Animation.Infinite
//        }
//    }
}
//! [2]
