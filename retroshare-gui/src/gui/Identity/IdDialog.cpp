<?xml version="1.0" encoding="UTF-8"?>
<ui version="4.0">
 <class>IdDialog</class>
 <widget class="QWidget" name="IdDialog">
  <property name="geometry">
   <rect>
    <x>0</x>
    <y>0</y>
    <width>800</width>
    <height>584</height>
   </rect>
  </property>
  <property name="sizePolicy">
   <sizepolicy hsizetype="Preferred" vsizetype="Preferred">
    <horstretch>0</horstretch>
    <verstretch>0</verstretch>
   </sizepolicy>
  </property>
  <property name="windowTitle">
   <string/>
  </property>
  <layout class="QGridLayout" name="IdDialogGLayout">
   <property name="leftMargin">
    <number>0</number>
   </property>
   <property name="topMargin">
    <number>0</number>
   </property>
   <property name="rightMargin">
    <number>0</number>
   </property>
   <property name="bottomMargin">
    <number>0</number>
   </property>
   <property name="horizontalSpacing">
    <number>6</number>
   </property>
   <item row="0" column="0">
    <widget class="QFrame" name="titleBarFrame">
     <property name="sizePolicy">
      <sizepolicy hsizetype="Preferred" vsizetype="Maximum">
       <horstretch>0</horstretch>
       <verstretch>0</verstretch>
      </sizepolicy>
     </property>
     <property name="frameShape">
      <enum>QFrame::NoFrame</enum>
     </property>
     <property name="frameShadow">
      <enum>QFrame::Sunken</enum>
     </property>
     <layout class="QHBoxLayout" name="titleBarFrameHLayout">
      <property name="leftMargin">
       <number>2</number>
      </property>
      <property name="topMargin">
       <number>2</number>
      </property>
      <property name="rightMargin">
       <number>2</number>
      </property>
      <property name="bottomMargin">
       <number>2</number>
      </property>
      <item>
       <widget class="QLabel" name="titleBarPixmap">
        <property name="minimumSize">
         <size>
          <width>0</width>
          <height>0</height>
         </size>
        </property>
        <property name="maximumSize">
         <size>
          <width>24</width>
          <height>24</height>
         </size>
        </property>
        <property name="text">
         <string/>
        </property>
        <property name="pixmap">
         <pixmap resource="../icons.qrc">:/icons/png/people2.png</pixmap>
        </property>
        <property name="scaledContents">
         <bool>true</bool>
        </property>
       </widget>
      </item>
      <item>
       <widget class="QLabel" name="titleBarLabel">
        <property name="font">
         <font>
          <pointsize>12</pointsize>
          <weight>75</weight>
          <bold>true</bold>
         </font>
        </property>
        <property name="text">
         <string>People</string>
        </property>
       </widget>
      </item>
      <item>
       <widget class="QLabel" name="label_count">
        <property name="text">
         <string>()</string>
        </property>
       </widget>
      </item>
      <item>
       <spacer name="titleBarSpacer">
        <property name="orientation">
         <enum>Qt::Horizontal</enum>
        </property>
        <property name="sizeHint" stdset="0">
         <size>
          <width>40</width>
          <height>20</height>
         </size>
        </property>
       </spacer>
      </item>
      <item>
       <widget class="QToolButton" name="helpButton">
        <property name="focusPolicy">
         <enum>Qt::NoFocus</enum>
        </property>
        <property name="icon">
         <iconset resource="../icons.qrc">
          <normaloff>:/icons/help_64.png</normaloff>:/icons/help_64.png</iconset>
        </property>
        <property name="checkable">
         <bool>true</bool>
        </property>
        <property name="autoRaise">
         <bool>true</bool>
        </property>
       </widget>
      </item>
     </layout>
    </widget>
   </item>
   <item row="2" column="0">
    <widget class="QSplitter" name="mainSplitter">
     <property name="orientation">
      <enum>Qt::Horizontal</enum>
     </property>
     <widget class="QWidget" name="layoutWidget">
      <layout class="QVBoxLayout" name="leftVLayout">
       <property name="spacing">
        <number>1</number>
       </property>
       <item>
        <widget class="QFrame" name="toolBarFrame">
         <property name="frameShape">
          <enum>QFrame::Box</enum>
         </property>
         <property name="frameShadow">
          <enum>QFrame::Sunken</enum>
         </property>
         <layout class="QHBoxLayout" name="toolBarFrameHLayout">
          <property name="leftMargin">
           <number>1</number>
          </property>
          <property name="topMargin">
           <number>1</number>
          </property>
          <property name="rightMargin">
           <number>1</number>
          </property>
          <property name="bottomMargin">
           <number>1</number>
          </property>
          <item>
           <widget class="LineEditClear" name="filterLineEdit"/>
          </item>
          <item>
           <widget class="QToolButton" name="toolButton_New">
            <property name="sizePolicy">
             <sizepolicy hsizetype="Preferred" vsizetype="Preferred">
              <horstretch>0</horstretch>
              <verstretch>0</verstretch>
             </sizepolicy>
            </property>
            <property name="focusPolicy">
             <enum>Qt::NoFocus</enum>
            </property>
            <property name="toolTip">
             <string>Create new...</string>
            </property>
            <property name="icon">
             <iconset resource="../icons.qrc">
              <normaloff>:/icons/png/add.png</normaloff>:/icons/png/add.png</iconset>
            </property>
            <property name="iconSize">
             <size>
              <width>24</width>
              <height>24</height>
             </size>
            </property>
            <property name="popupMode">
             <enum>QToolButton::InstantPopup</enum>
            </property>
            <property name="autoRaise">
             <bool>true</bool>
            </property>
           </widget>
          </item>
         </layout>
        </widget>
       </item>
       <item>
        <widget class="RSTreeWidget" name="idTreeWidget">
         <property name="sizePolicy">
          <sizepolicy hsizetype="Preferred" vsizetype="Expanding">
           <horstretch>0</horstretch>
           <verstretch>0</verstretch>
          </sizepolicy>
         </property>
         <property name="font">
          <font>
           <pointsize>11</pointsize>
          </font>
         </property>
         <property name="contextMenuPolicy">
          <enum>Qt::CustomContextMenu</enum>
         </property>
         <property name="selectionMode">
          <enum>QAbstractItemView::ExtendedSelection</enum>
         </property>
         <property name="iconSize">
          <size>
           <width>22</width>
           <height>22</height>
          </size>
         </property>
         <property name="indentation">
          <number>24</number>
         </property>
         <property name="sortingEnabled">
          <bool>true</bool>
         </property>
         <attribute name="headerStretchLastSection">
          <bool>false</bool>
         </attribute>
         <column>
          <property name="text">
           <string>Persons</string>
          </property>
         </column>
         <column>
          <property name="text">
           <string>Identity ID</string>
          </property>
         </column>
         <column>
          <property name="text">
           <string>Owned by</string>
          </property>
         </column>
         <column>
          <property name="text">
           <string/>
          </property>
          <property name="toolTip">
           <string>Votes</string>
          </property>
          <property name="textAlignment">
           <set>AlignLeading|AlignVCenter</set>
          </property>
          <property name="icon">
           <iconset resource="../icons.qrc">
            <normaloff>:/icons/flag-green.png</normaloff>:/icons/flag-green.png</iconset>
          </property>
         </column>
        </widget>
       </item>
      </layout>
     </widget>
     <widget class="QTabWidget" name="rightTabWidget">
      <property name="currentIndex">
       <number>0</number>
      </property>
      <widget class="QWidget" name="personTab">
       <attribute name="icon">
        <iconset resource="../icons.qrc">
         <normaloff>:/icons/png/person.png</normaloff>:/icons/png/person.png</iconset>
       </attribute>
       <attribute name="title">
        <string>Person</string>
       </attribute>
       <layout class="QVBoxLayout" name="personTabVLayout">
        <item>
         <widget class="QScrollArea" name="scrollArea">
          <property name="widgetResizable">
           <bool>true</bool>
          </property>
          <widget class="QWidget" name="scrollAreaWidgetContents">
           <property name="geometry">
            <rect>
             <x>0</x>
             <y>0</y>
             <width>466</width>
             <height>738</height>
            </rect>
           </property>
           <layout class="QVBoxLayout" name="scrollAreaWidgetContentsVLayout">
            <item>
             <widget class="QFrame" name="headerBFramePerson">
              <property name="sizePolicy">
               <sizepolicy hsizetype="Preferred" vsizetype="Maximum">
                <horstretch>0</horstretch>
                <verstretch>0</verstretch>
               </sizepolicy>
              </property>
              <property name="frameShape">
               <enum>QFrame::StyledPanel</enum>
              </property>
              <property name="frameShadow">
               <enum>QFrame::Raised</enum>
              </property>
              <layout class="QGridLayout" name="headerBFramePersonGLayout">
               <property name="horizontalSpacing">
                <number>12</number>
               </property>
               <item row="0" column="3">
                <layout class="QVBoxLayout" name="headerFramePersonVLayout">
                 <item>
                  <widget class="ElidedLabel" name="headerTextLabel_Person">
                   <property name="font">
                    <font>
                     <pointsize>22</pointsize>
                    </font>
                   </property>
                   <property name="text">
                    <string>People</string>
                   </property>
                  </widget>
                 </item>
                 <item>
                  <widget class="QFrame" name="info_Frame_Invite">
                   <property name="sizePolicy">
                    <sizepolicy hsizetype="Expanding" vsizetype="Preferred">
                     <horstretch>0</horstretch>
                     <verstretch>0</verstretch>
                    </sizepolicy>
                   </property>
                   <property name="palette">
                    <palette>
                     <active>
                      <colorrole role="WindowText">
                       <brush brushstyle="SolidPattern">
                        <color alpha="255">
                         <red>0</red>
                         <green>0</green>
                         <blue>0</blue>
                        </color>
                       </brush>
                      </colorrole>
                      <colorrole role="Base">
                       <brush brushstyle="SolidPattern">
                        <color alpha="255">
                         <red>255</red>
                         <green>255</green>
                         <blue>255</blue>
                        </color>
                       </brush>
                      </colorrole>
                      <colorrole role="Window">
                       <brush brushstyle="SolidPattern">
                        <color alpha="255">
                         <red>255</red>
                         <green>255</green>
                         <blue>178</blue>
                        </color>
                       </brush>
                      </colorrole>
                     </active>
                     <inactive>
                      <colorrole role="WindowText">
                       <brush brushstyle="SolidPattern">
                        <color alpha="255">
                         <red>0</red>
                         <green>0</green>
                         <blue>0</blue>
                        </color>
                       </brush>
                      </colorrole>
                      <colorrole role="Base">
                       <brush brushstyle="SolidPattern">
                        <color alpha="255">
                         <red>255</red>
                         <green>255</green>
                         <blue>255</blue>
                        </color>
                       </brush>
                      </colorrole>
                      <colorrole role="Window">
                       <brush brushstyle="SolidPattern">
                        <color alpha="255">
                         <red>255</red>
                         <green>255</green>
                         <blue>178</blue>
                        </color>
                       </brush>
                      </colorrole>
                     </inactive>
                     <disabled>
                      <colorrole role="WindowText">
                       <brush brushstyle="SolidPattern">
                        <color alpha="255">
                         <red>154</red>
                         <green>154</green>
                         <blue>154</blue>
                        </color>
                       </brush>
                      </colorrole>
                      <colorrole role="Base">
                       <brush brushstyle="SolidPattern">
                        <color alpha="255">
                         <red>255</red>
                         <green>255</green>
                         <blue>178</blue>
                        </color>
                       </brush>
                      </colorrole>
                      <colorrole role="Window">
                       <brush brushstyle="SolidPattern">
                        <color alpha="255">
                         <red>255</red>
                         <green>255</green>
                         <blue>178</blue>
                        </color>
                       </brush>
                      </colorrole>
                     </disabled>
                    </palette>
                   </property>
                   <property name="autoFillBackground">
                    <bool>true</bool>
                   </property>
                   <property name="styleSheet">
                    <string notr="true"/>
                   </property>
                   <property name="frameShape">
                    <enum>QFrame::Box</enum>
                   </property>
                   <layout class="QHBoxLayout" name="info_Frame_Invite_HL">
                    <property name="leftMargin">
                     <number>6</number>
                    </property>
                    <property name="topMargin">
                     <number>6</number>
                    </property>
                    <property name="rightMargin">
                     <number>6</number>
                    </property>
                    <property name="bottomMargin">
                     <number>6</number>
                    </property>
                    <item>
                     <widget class="QLabel" name="infoPixmap_Invite">
                      <property name="maximumSize">
                       <size>
                        <width>16</width>
                        <height>16</height>
                       </size>
                      </property>
                      <property name="text">
                       <string/>
                      </property>
                      <property name="pixmap">
                       <pixmap resource="../images.qrc">:/images/info16.png</pixmap>
                      </property>
                      <property name="alignment">
                       <set>Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop</set>
                      </property>
                     </widget>
                    </item>
                    <item>
                     <widget class="QLabel" name="infoLabel_Invite">
                      <property name="text">
                       <string notr="true">Invite messages stay into your Outbox until an acknowledgement of receipt has been received.</string>
                      </property>
                      <property name="wordWrap">
                       <bool>true</bool>
                      </property>
                     </widget>
                    </item>
                    <item>
                     <widget class="QToolButton" name="closeInfoFrameButton_Invite">
                      <property name="maximumSize">
                       <size>
                        <width>16</width>
                        <height>16</height>
                       </size>
                      </property>
                      <property name="focusPolicy">
                       <enum>Qt::NoFocus</enum>
                      </property>
                      <property name="toolTip">
                       <string>Close</string>
                      </property>
                      <property name="styleSheet">
                       <string notr="true">QToolButton
{
 border-image: url(:/images/closenormal.png) 
}
                                
QToolButton:hover 
{
border-image: url(:/images/closehover.png) 
}
                                
QToolButton:pressed  {
border-image: url(:/images/closepressed.png) 
}</string>
                      </property>
                      <property name="autoRaise">
                       <bool>true</bool>
                      </property>
                     </widget>
                    </item>
                   </layout>
                  </widget>
                 </item>
                </layout>
               </item>
               <item row="0" column="0" rowspan="2">
                <widget class="QLabel" name="avLabel_Person">
                 <property name="sizePolicy">
                  <sizepolicy hsizetype="Maximum" vsizetype="Maximum">
                   <horstretch>0</horstretch>
                   <verstretch>0</verstretch>
                  </sizepolicy>
                 </property>
                 <property name="minimumSize">
                  <size>
                   <width>64</width>
                   <height>64</height>
                  </size>
                 </property>
                 <property name="maximumSize">
                  <size>
                   <width>1000</width>
                   <height>1000</height>
                  </size>
                 </property>
                 <property name="styleSheet">
                  <string notr="true"/>
                 </property>
                 <property name="text">
                  <string/>
                 </property>
                 <property name="scaledContents">
                  <bool>true</bool>
                 </property>
                </widget>
               </item>
              </layout>
             </widget>
            </item>
            <item>
             <widget class="QGroupBox" name="detailsGroupBox">
              <property name="minimumSize">
               <size>
                <width>0</width>
                <height>299</height>
               </size>
              </property>
              <property name="title">
               <string>Identity info</string>
              </property>
              <layout class="QGridLayout" name="gridLayout">
               <item row="13" column="0">
                <widget class="QLabel" name="neighborNodesOpinion_LB">
                 <property name="text">
                  <string>Friend votes:</string>
                 </property>
                </widget>
               </item>
               <item row="9" column="0" colspan="2">
                <widget class="Line" name="line_IdInfo">
                 <property name="orientation">
                  <enum>Qt::Horizontal</enum>
                 </property>
                </widget>
               </item>
               <item row="4" column="1">
                <widget class="QLineEdit" name="lineEdit_GpgName">
                 <property name="enabled">
                  <bool>true</bool>
                 </property>
                 <property name="readOnly">
                  <bool>true</bool>
                 </property>
                </widget>
               </item>
               <item row="1" column="1">
                <widget class="QLineEdit" name="lineEdit_KeyId">
                 <property name="enabled">
                  <bool>true</bool>
                 </property>
                 <property name="readOnly">
                  <bool>true</bool>
                 </property>
                </widget>
               </item>
               <item row="6" column="0">
                <widget class="QLabel" name="label_LastUsed">
                 <property name="text">
                  <string>Last used:</string>
                 </property>
                </widget>
               </item>
               <item row="12" column="1">
                <widget class="QCheckBox" name="autoBanIdentities_CB">
                 <property name="toolTip">
                  <string>Auto-Ban all identities signed by the same node</string>
                 </property>
                 <property name="text">
                  <string>Auto-Ban profile</string>
                 </property>
                </widget>
               </item>
               <item row="5" column="1">
                <widget class="QLineEdit" name="lineEdit_PublishTS">
                 <property name="readOnly">
                  <bool>true</bool>
                 </property>
                </widget>
               </item>
               <item row="0" column="1">
                <widget class="QLineEdit" name="lineEdit_Nickname">
                 <property name="enabled">
                  <bool>true</bool>
                 </property>
                 <property name="readOnly">
                  <bool>true</bool>
                 </property>
                </widget>
               </item>
               <item row="3" column="1">
                <widget class="QLineEdit" name="lineEdit_GpgId">
                 <property name="enabled">
                  <bool>true</bool>
                 </property>
                 <property name="readOnly">
                  <bool>true</bool>
                 </property>
                </widget>
               </item>
               <item row="14" column="1">
                <widget class="QLineEdit" name="overallOpinion_TF">
                 <property name="toolTip">
                  <string>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Overall reputation score, accounting for yours and your friends'.&lt;/p&gt;&lt;p&gt;Negative is bad, positive is good. Zero is neutral. If the score is too low,&lt;/p&gt;&lt;p&gt;the identity is flagged as bad, and will be filtered out in forums, chat lobbies,&lt;/p&gt;&lt;p&gt;channels, etc.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</string>
                 </property>
                 <property name="readOnly">
                  <bool>true</bool>
                 </property>
                </widget>
               </item>
               <item row="10" column="0" rowspan="2">
                <widget class="QLabel" name="label_YourOpinion">
                 <property name="text">
                  <string>Your opinion:</string>
                 </property>
                </widget>
               </item>
               <item row="14" column="0">
                <widget class="QLabel" name="overallOpinion_LB">
                 <property name="font">
                  <font>
                   <weight>75</weight>
                   <bold>true</bold>
                  </font>
                 </property>
                 <property name="text">
                  <string>Overall:</string>
                 </property>
                </widget>
               </item>
               <item row="5" column="0">
                <widget class="QLabel" name="label_PublishTS">
                 <property name="text">
                  <string>Created on :</string>
                 </property>
                </widget>
               </item>
               <item row="0" column="0">
                <widget class="QLabel" name="label_Nickname">
                 <property name="text">
                  <string>Identity name :</string>
                 </property>
                </widget>
               </item>
               <item row="1" column="0">
                <widget class="QLabel" name="label_KeyId">
                 <property name="text">
                  <string>Identity ID :</string>
                 </property>
                </widget>
               </item>
               <item row="13" column="1">
                <widget class="QLineEdit" name="neighborNodesOpinion_TF">
                 <property name="toolTip">
                  <string>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Average opinion of neighbor nodes about this identity. Negative is bad,&lt;/p&gt;&lt;p&gt;positive is good. Zero is neutral.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</string>
                 </property>
                 <property name="readOnly">
                  <bool>true</bool>
                 </property>
                </widget>
               </item>
               <item row="2" column="1">
                <widget class="QLineEdit" name="lineEdit_Type">
                 <property name="readOnly">
                  <bool>true</bool>
                 </property>
                </widget>
               </item>
               <item row="2" column="0">
                <widget class="QLabel" name="label_Type">
                 <property name="text">
                  <string>Type:</string>
                 </property>
                </widget>
               </item>
               <item row="6" column="1">
                <widget class="QLineEdit" name="lineEdit_LastUsed">
                 <property name="readOnly">
                  <bool>true</bool>
                 </property>
                </widget>
               </item>
               <item row="15" column="0" colspan="2">
                <spacer name="verticalSpacer">
                 <property name="orientation">
                  <enum>Qt::Vertical</enum>
                 </property>
                 <property name="sizeHint" stdset="0">
                  <size>
                   <width>20</width>
                   <height>1</height>
                  </size>
                 </property>
                </spacer>
               </item>
               <item row="3" column="0">
                <widget class="QLabel" name="label_GpgId">
                 <property name="text">
                  <string>Owner node ID :</string>
                 </property>
                </widget>
               </item>
               <item row="4" column="0">
                <widget class="QLabel" name="label_GpgName">
                 <property name="text">
                  <string>Owner node name :</string>
                 </property>
                </widget>
               </item>
               <item row="12" column="0">
                <widget class="QLabel" name="banoption_label">
                 <property name="text">
                  <string>Ban-option:</string>
                 </property>
                </widget>
               </item>
               <item row="10" column="1" rowspan="2">
                <widget class="RSComboBox" name="ownOpinion_CB">
                 <property name="toolTip">
                  <string>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;&lt;span style=&quot; font-family:'Sans'; font-size:9pt;&quot;&gt;Your own opinion about an identity rules the visibility of that identity for yourself and your friend nodes. Your own opinion is shared among friends and used to compute a reputation score: If your opinion about an identity is neutral, the reputation score is the difference between friend's positive and negative opinions. If not, your own opinion gives the score.&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-family:'Sans'; font-size:9pt;&quot;&gt;The overall score is used in chat lobbies, forums and channels to decide on the actions to take for each specific identity. When the overall score is lower than -1, the identity is banned, which prevents all messages and forums/channels authored by this identity to be forwarded, both ways. Some forums also have special anti-spam flags that require a non negative reputation level, making them more sensitive to bad opinions. Banned identities gradually lose their activity and eventually disappear (after 5 days).&lt;/span&gt;&lt;/p&gt;&lt;p&gt;&lt;span style=&quot; font-family:'Sans'; font-size:9pt;&quot;&gt;You can change the thresholds and the time of inactivity to delete identities in preferences -&amp;gt; people. &lt;/span&gt;&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</string>
                 </property>
                 <property name="iconSize">
                  <size>
                   <width>22</width>
                   <height>22</height>
                  </size>
                 </property>
                 <item>
                  <property name="text">
                   <string>Negative</string>
                  </property>
                  <property name="icon">
                   <iconset resource="../icons.qrc">
                    <normaloff>:/icons/png/thumbs-down.png</normaloff>:/icons/png/thumbs-down.png</iconset>
                  </property>
                 </item>
                 <item>
                  <property name="text">
                   <string>Neutral</string>
                  </property>
                  <property name="icon">
                   <iconset resource="../icons.qrc">
                    <normaloff>:/icons/png/thumbs-neutral.png</normaloff>:/icons/png/thumbs-neutral.png</iconset>
                  </property>
                 </item>
                 <item>
                  <property name="text">
                   <string>Positive</string>
                  </property>
                  <property name="icon">
                   <iconset resource="../icons.qrc">
                    <normaloff>:/icons/png/thumbs-up.png</normaloff>:/icons/png/thumbs-up.png</iconset>
                  </property>
                 </item>
                </widget>
               </item>
               <item row="0" column="2" rowspan="15">
                <layout class="QVBoxLayout" name="verticalLayout">
                 <property name="spacing">
                  <number>0</number>
                 </property>
                 <item>
                  <layout class="QGridLayout" name="gridLayout_2">
                   <item row="0" column="1">
                    <widget class="QLabel" name="avatarLabel">
                     <property name="sizePolicy">
                      <sizepolicy hsizetype="Fixed" vsizetype="Fixed">
                       <horstretch>0</horstretch>
                       <verstretch>0</verstretch>
                      </sizepolicy>
                     </property>
                     <property name="minimumSize">
                      <size>
                       <width>128</width>
                       <height>128</height>
                      </size>
                     </property>
                     <property name="maximumSize">
                      <size>
                       <width>128</width>
                       <height>128</height>
                      </size>
                     </property>
                     <property name="frameShape">
                      <enum>QFrame::Box</enum>
                     </property>
                     <property name="frameShadow">
                      <enum>QFrame::Sunken</enum>
                     </property>
                     <property name="text">
                      <string extracomment="Click here to change your avatar">Your Avatar</string>
                     </property>
                     <property name="scaledContents">
                      <bool>true</bool>
                     </property>
                     <property name="alignment">
                      <set>Qt::AlignCenter</set>
                     </property>
                    </widget>
                   </item>
                   <item row="2" column="1">
                    <widget class="QPushButton" name="editButton">
                     <property name="text">
                      <string>Edit Identity</string>
                     </property>
                    </widget>
                   </item>
                   <item row="1" column="1">
                    <widget class="QPushButton" name="inviteButton">
                     <property name="text">
                      <string>Send Invite</string>
                     </property>
                    </widget>
                   </item>
                   <item row="0" column="2" rowspan="3">
                    <spacer name="horizontalSpacer">
                     <property name="orientation">
                      <enum>Qt::Horizontal</enum>
                     </property>
                     <property name="sizeType">
                      <enum>QSizePolicy::Minimum</enum>
                     </property>
                     <property name="sizeHint" stdset="0">
                      <size>
                       <width>6</width>
                       <height>128</height>
                      </size>
                     </property>
                    </spacer>
                   </item>
                   <item row="0" column="0" rowspan="3">
                    <spacer name="horizontalSpacer_2">
                     <property name="orientation">
                      <enum>Qt::Horizontal</enum>
                     </property>
                     <property name="sizeType">
                      <enum>QSizePolicy::Minimum</enum>
                     </property>
                     <property name="sizeHint" stdset="0">
                      <size>
                       <width>6</width>
                       <height>128</height>
                      </size>
                     </property>
                    </spacer>
                   </item>
                  </layout>
                 </item>
                 <item>
                  <layout class="QHBoxLayout" name="avatarOpinionHLayout">
                   <property name="spacing">
                    <number>6</number>
                   </property>
                   <property name="topMargin">
                    <number>2</number>
                   </property>
                   <item>
                    <widget class="QLabel" name="label_PosIcon">
                     <property name="maximumSize">
                      <size>
                       <width>34</width>
                       <height>34</height>
                      </size>
                     </property>
                     <property name="text">
                      <string/>
                     </property>
                     <property name="pixmap">
                      <pixmap resource="../icons.qrc">:/icons/png/thumbs-up.png</pixmap>
                     </property>
                     <property name="scaledContents">
                      <bool>true</bool>
                     </property>
                    </widget>
                   </item>
                   <item>
                    <widget class="QLabel" name="label_positive">
                     <property name="font">
                      <font>
                       <pointsize>16</pointsize>
                      </font>
                     </property>
                     <property name="toolTip">
                      <string>Positive votes</string>
                     </property>
                     <property name="text">
                      <string>0</string>
                     </property>
                    </widget>
                   </item>
                   <item>
                    <widget class="Line" name="line_Opinion">
                     <property name="orientation">
                      <enum>Qt::Vertical</enum>
                     </property>
                    </widget>
                   </item>
                   <item>
                    <widget class="QLabel" name="label_NegIcon">
                     <property name="maximumSize">
                      <size>
                       <width>34</width>
                       <height>34</height>
                      </size>
                     </property>
                     <property name="text">
                      <string/>
                     </property>
                     <property name="pixmap">
                      <pixmap resource="../icons.qrc">:/icons/png/thumbs-down.png</pixmap>
                     </property>
                     <property name="scaledContents">
                      <bool>true</bool>
                     </property>
                    </widget>
                   </item>
                   <item>
                    <widget class="QLabel" name="label_negative">
                     <property name="font">
                      <font>
                       <pointsize>16</pointsize>
                      </font>
                     </property>
                     <property name="toolTip">
                      <string>Negative votes</string>
                     </property>
                     <property name="text">
                      <string>0</string>
                     </property>
                    </widget>
                   </item>
                  </layout>
                 </item>
                 <item>
                  <spacer name="avatarVSpacer">
                   <property name="orientation">
                    <enum>Qt::Vertical</enum>
                   </property>
                   <property name="sizeHint" stdset="0">
                    <size>
                     <width>118</width>
                     <height>17</height>
                    </size>
                   </property>
                  </spacer>
                 </item>
                </layout>
               </item>
              </layout>
             </widget>
            </item>
            <item>
             <widget class="QGroupBox" name="usageStatisticsGBox">
              <property name="title">
               <string>Usage statistics</string>
              </property>
              <layout class="QHBoxLayout" name="usageStatisticsGBoxHLayout">
               <item>
                <widget class="RSTextBrowser" name="usageStatistics_TB"/>
               </item>
              </layout>
             </widget>
            </item>
            <item>
             <spacer name="scrollAreaWidgetContentsVSpacer">
              <property name="orientation">
               <enum>Qt::Vertical</enum>
              </property>
              <property name="sizeHint" stdset="0">
               <size>
                <width>20</width>
                <height>40</height>
               </size>
              </property>
             </spacer>
            </item>
           </layout>
           <zorder>detailsGroupBox</zorder>
           <zorder>usageStatisticsGBox</zorder>
           <zorder>headerBFramePerson</zorder>
          </widget>
         </widget>
        </item>
       </layout>
      </widget>
      <widget class="QWidget" name="circleTab">
       <attribute name="icon">
        <iconset resource="../icons.qrc">
         <normaloff>:/icons/png/circles.png</normaloff>:/icons/png/circles.png</iconset>
       </attribute>
       <attribute name="title">
        <string>Circles</string>
       </attribute>
       <layout class="QVBoxLayout" name="circleTabVLayout">
        <item>
         <widget class="QFrame" name="headerBFrameCircle">
          <property name="frameShape">
           <enum>QFrame::StyledPanel</enum>
          </property>
          <property name="frameShadow">
           <enum>QFrame::Raised</enum>
          </property>
          <layout class="QGridLayout" name="headerBFrameCircleGLayout">
           <property name="horizontalSpacing">
            <number>12</number>
           </property>
           <item row="0" column="0" rowspan="2">
            <widget class="QLabel" name="avLabel_Circles">
             <property name="minimumSize">
              <size>
               <width>64</width>
               <height>64</height>
              </size>
             </property>
             <property name="maximumSize">
              <size>
               <width>64</width>
               <height>64</height>
              </size>
             </property>
             <property name="styleSheet">
              <string notr="true"/>
             </property>
             <property name="text">
              <string/>
             </property>
             <property name="scaledContents">
              <bool>true</bool>
             </property>
            </widget>
           </item>
           <item row="0" column="1" rowspan="2" colspan="2">
            <widget class="ElidedLabel" name="headerTextLabel_Circles">
             <property name="font">
              <font>
               <pointsize>22</pointsize>
              </font>
             </property>
             <property name="text">
              <string>Circles</string>
             </property>
            </widget>
           </item>
          </layout>
         </widget>
        </item>
        <item>
         <widget class="QTreeWidget" name="treeWidget_membership">
          <property name="font">
           <font>
            <pointsize>11</pointsize>
           </font>
          </property>
          <property name="contextMenuPolicy">
           <enum>Qt::CustomContextMenu</enum>
          </property>
          <property name="sortingEnabled">
           <bool>true</bool>
          </property>
          <column>
           <property name="text">
            <string>Circle name</string>
           </property>
          </column>
          <column>
           <property name="text">
            <string>Membership</string>
           </property>
          </column>
          <item>
           <property name="text">
            <string>Public Circles</string>
           </property>
          </item>
          <item>
           <property name="text">
            <string>Personal Circles</string>
           </property>
          </item>
         </widget>
        </item>
       </layout>
      </widget>
     </widget>
    </widget>
   </item>
  </layout>
  <action name="editIdentity">
   <property name="text">
    <string>Edit identity</string>
   </property>
   <property name="toolTip">
    <string>Edit identity</string>
   </property>
  </action>
  <action name="removeIdentity">
   <property name="text">
    <string>Delete identity</string>
   </property>
  </action>
  <action name="chatIdentity">
   <property name="text">
    <string>Chat with this peer</string>
   </property>
   <property name="toolTip">
    <string>Launches a distant chat with this peer</string>
   </property>
  </action>
 </widget>
 <customwidgets>
  <customwidget>
   <class>LineEditClear</class>
   <extends>QLineEdit</extends>
   <header location="global">gui/common/LineEditClear.h</header>
  </customwidget>
  <customwidget>
   <class>RSTreeWidget</class>
   <extends>QTreeWidget</extends>
   <header>gui/common/RSTreeWidget.h</header>
  </customwidget>
  <customwidget>
   <class>ElidedLabel</class>
   <extends>QLabel</extends>
   <header>gui/common/ElidedLabel.h</header>
  </customwidget>
  <customwidget>
   <class>RSComboBox</class>
   <extends>QComboBox</extends>
   <header>gui/common/RSComboBox.h</header>
  </customwidget>
  <customwidget>
   <class>RSTextBrowser</class>
   <extends>QTextBrowser</extends>
   <header>gui/common/RSTextBrowser.h</header>
  </customwidget>
 </customwidgets>
 <tabstops>
  <tabstop>idTreeWidget</tabstop>
 </tabstops>
 <resources>
  <include location="../icons.qrc"/>
  <include location="../images.qrc"/>
 </resources>
 <connections/>
</ui>
