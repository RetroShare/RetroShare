<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="de">
<context>
    <name>AudioInput</name>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="+17"/>
        <source>Audio Wizard</source>
        <translation>Audio-Assistent</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Transmission</source>
        <translation>Übertragung</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&amp;Transmit</source>
        <translation>Über&amp;tragen</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>When to transmit your speech</source>
        <translation>Wann Sprache übertragen werden soll</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets when speech should be transmitted.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuous&lt;/i&gt; - All the time&lt;br /&gt;&lt;i&gt;Voice Activity&lt;/i&gt; - When you are speaking clearly.&lt;br /&gt;&lt;i&gt;Push To Talk&lt;/i&gt; - When you hold down the hotkey set under &lt;i&gt;Shortcuts&lt;/i&gt;.</source>
        <translation>&lt;b&gt;Dies legt fest, wann Sprache übertragen werden soll.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Kontinuierlich&lt;/i&gt; - Die ganze Zeit&lt;br /&gt;&lt;i&gt;Stimmaktivierung&lt;/i&gt; - Sobald man deutlich spricht.&lt;/br&gt;&lt;i&gt;Sendeumschaltung&lt;/i&gt; - Wenn ein Hotkey gedrückt wird (siehe &lt;i&gt;Shortcuts&lt;/i&gt;).</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>DoublePush Time</source>
        <translation>Doppeldrück-Zeit</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>If you press the PTT key twice in this time it will get locked.</source>
        <translation>Wenn du die Sprechtaste (Push-to-Talk) zweimal innerhalb dieser Zeit anklickst, wird die Sprachübertragung dauerhaft aktiviert.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;DoublePush Time&lt;/b&gt;&lt;br /&gt;If you press the push-to-talk key twice during the configured interval of time it will be locked. RetroShare will keep transmitting until you hit the key once more to unlock PTT again.</source>
        <translation>&lt;b&gt;Doppeldrück-Zeit&lt;/b&gt;&lt;br /&gt;Wenn du die Sprechtaste (Push-to-Talk) zweimal innerhalb des eingestellten Zeitintervalls anklickst, wird die Sprachübertragung dauerhaft aktiviert. RetroShare überträgt solange Daten, bis du die Sprechtaste ein weiteres Mal anklickst.</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Voice &amp;Hold</source>
        <translation>Stimme &amp;halten</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>How long to keep transmitting after silence</source>
        <translation>Wie lange nach dem Einsetzen von Stille übertragen werden soll</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This selects how long after a perceived stop in speech transmission should continue.&lt;/b&gt;&lt;br /&gt;Set this higher if your voice breaks up when you speak (seen by a rapidly blinking voice icon next to your name).</source>
        <translation>&lt;b&gt;Hiermit bestimmst du, wie lange nach Beenden des Gesprächs noch übertragen werden soll.&lt;/b&gt;&lt;br /&gt;Höhere Werte sind hilfreich, wenn die Stimme plötzlich abbricht (erkennbar an einem flackerndem Voice-Icon neben dem Namen).</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Silence Below</source>
        <translation>Stille bis</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values below this count as silence</source>
        <translation>Signalwerte darunter zählen als Stille</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+32"/>
        <source>&lt;b&gt;This sets the trigger values for voice detection.&lt;/b&gt;&lt;br /&gt;Use this together with the Audio Statistics window to manually tune the trigger values for detecting speech. Input values below &quot;Silence Below&quot; always count as silence. Values above &quot;Speech Above&quot; always count as voice. Values in between will count as voice if you&apos;re already talking, but will not trigger a new detection.</source>
        <translation>&lt;b&gt;Dies legt die Auslösewerte für die Spracherkennung fest.&lt;/b&gt;&lt;br /&gt;Zusammen mit dem Audiostatistikfenster können die Auslösewerte für die Spracherkennung manuell eingestellt werden. Eingabewerte unter &quot;Stille bis&quot; zählen immer als Stille, Werte über &quot;Sprache über&quot; immer als Sprache. Werte dazwischen zählen als Sprache wenn schon gesprochen wird, lösen aber keine Erkennung (und damit Übertragung) aus.</translation>
    </message>
    <message>
        <location line="-10"/>
        <source>Speech Above</source>
        <translation>Sprache über</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values above this count as voice</source>
        <translation>Signalwerte darüber zählen als Sprache</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>empty</source>
        <translation>leer</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Audio Processing</source>
        <translation>Audioverarbeitung</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Noise Suppression</source>
        <translation>Rauschunterdrückung</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Noise suppression</source>
        <translation>Rauschunterdrückung</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets the amount of noise suppression to apply.&lt;/b&gt;&lt;br /&gt;The higher this value, the more aggressively stationary noise will be suppressed.</source>
        <translation>&lt;b&gt;Dies setzt die Stärke der Rauschunterdrückung die angewandt werden soll&lt;/b&gt;&lt;br /&gt;Je höher der Wert, desto aggressiver wird Rauschen unterdrückt.</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Amplification</source>
        <translation>Verstärkung</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Maximum amplification of input sound</source>
        <translation>Maximale Verstärkung des Eingangssignals</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;Maximum amplification of input.&lt;/b&gt;&lt;br /&gt;RetroShare normalizes the input volume before compressing, and this sets how much it&apos;s allowed to amplify.&lt;br /&gt;The actual level is continually updated based on your current speech pattern, but it will never go above the level specified here.&lt;br /&gt;If the &lt;i&gt;Microphone loudness&lt;/i&gt; level of the audio statistics hover around 100%, you probably want to set this to 2.0 or so, but if, like most people, you are unable to reach 100%, set this to something much higher.&lt;br /&gt;Ideally, set it so &lt;i&gt;Microphone Loudness * Amplification Factor &gt;= 100&lt;/i&gt;, even when you&apos;re speaking really soft.&lt;br /&gt;&lt;br /&gt;Note that there is no harm in setting this to maximum, but RetroShare will start picking up other conversations if you leave it to auto-tune to that level.</source>
        <translation>&lt;b&gt;Maximale Verstärkung des Eingangssignals.&lt;/b&gt;&lt;br /&gt;RetroShare normalisiert die Eingangslautstärke vor der Kompression, wobei diese Option festlegt wie sehr verstärkt werden darf.&lt;br /&gt;Der tatsächliche Level wird kontinuierlich, abhängig vom Sprachmuster, aktualisiert; allerdings nie höher als hier festgelegt.&lt;br /&gt;Wenn die Mikrofonlautstärke in den Audiostatistiken um 100% liegt, sollte man dies auf 2.0 setzen. Für Leute die dies kaum erreichen, muss es deutlich höher angesetzt werden.&lt;br /&gt;Idealerweise sollte es folgendermaßen gesetzt werden: &lt;i&gt;Mikrofon Lautstärke * Verstärkungsfaktor &gt;= 100&lt;/i&gt;, selbst wenn man wirklich leise spricht.&lt;br /&gt;Es ist nicht schädlich dies auf das Maximum zu setzen, aber RetroShare wird dadurch auch Umgebungsgeräusche aufnehmen.</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Echo Cancellation Processing</source>
        <translation>Echokompensationsverarbeitung</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video Processing</source>
        <translation>Videoverarbeitung</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Available bandwidth:</source>
        <translation>Verfügbare Bandbreite:</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Use this field to simulate the maximum bandwidth available so as to preview what the encoded video will look like with the corresponding compression rate.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>KB/s</source>
        <translation>KB/s</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Display encoded (and then decoded) frame, to check the codec&apos;s quality. If not selected, the image above only shows the frame that is grabbed from your camera.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>preview</source>
        <translation>Vorschau</translation>
    </message>
</context>
<context>
    <name>AudioInputConfig</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="+185"/>
        <source>Continuous</source>
        <translation>Kontinuierlich</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Voice Activity</source>
        <translation>Stimmaktivität</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Push To Talk</source>
        <translation>Zum Sprechen drücken</translation>
    </message>
    <message>
        <location line="+91"/>
        <source>%1 s</source>
        <translation>%1 s</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Off</source>
        <translation>Aus</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>-%1 dB</source>
        <translation>-%1 dB</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.h" line="+85"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
</context>
<context>
    <name>AudioStats</name>
    <message>
        <location filename="../gui/AudioStats.ui" line="+14"/>
        <source>Audio Statistics</source>
        <translation>Audiostatistiken</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Input Levels</source>
        <translation>Eingangslevel</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Peak microphone level</source>
        <translation>Mikrofonspitzenpegel</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+20"/>
        <location line="+20"/>
        <source>Peak power in last frame</source>
        <translation>Spitzenpegel im letzten Frame</translation>
    </message>
    <message>
        <location line="-37"/>
        <source>This shows the peak power in the last frame (20 ms), and is the same measurement as you would usually find displayed as &quot;input power&quot;. Please disregard this and look at &lt;b&gt;Microphone power&lt;/b&gt; instead, which is much more steady and disregards outliers.</source>
        <translation>Dies zeigt den Spitzenpegel im letzten Frame (20 ms) an und ist derselbe Wert, den man üblicherweise als &quot;Eingangspegel&quot; angezeigt bekommt. Bitte ignoriere diesen Wert und verwende &lt;b&gt;Mikrofonpegel&lt;/b&gt;. Dieser Wert ist stabiler und ignoriert extreme Ausschläge.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak speaker level</source>
        <translation>Lautsprecherspitzenpegel</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power of the speakers in the last frame (20 ms). Unless you are using a multi-channel sampling method (such as ASIO) with speaker channels configured, this will be 0. If you have such a setup configured, and this still shows 0 while you&apos;re playing audio from other programs, your setup is not working.</source>
        <translation>Dies zeigt den Spitzenpegel der Lautsprecher im letzten Frame (20 ms). Sofern du nicht eine Multi-Channel-Sampling-Methode (wie ASIO) mit konfigurierten Lautsprecherkanälen benutzt, wird dies 0 sein. Wenn du eine solche Installation konfiguriert hast und dies immer noch 0 zeigt, während du mit anderen Programmen Audio abspielst, funktioniert dein Setup nicht.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak clean level</source>
        <translation>Bereinigter Spitzenpegel</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power in the last frame (20 ms) after all processing. Ideally, this should be -96 dB when you&apos;re not talking. In reality, a sound studio should see -60 dB, and you should hopefully see somewhere around -20 dB. When you are talking, this should rise to somewhere between -5 and -10 dB.&lt;br /&gt;If you are using echo cancellation, and this rises to more than -15 dB when you&apos;re not talking, your setup is not working, and you&apos;ll annoy other users with echoes.</source>
        <translation>Dies zeigt den Spitzenpegel im letzten Frame (20 ms), nach allen Verarbeitungsschritten. Im Idealfall sollte dies -96 dB sein, wenn du nicht sprichst. In der Realität würde ein Tonstudio -60 dB anzeigen, und du wirst hoffentlich irgendetwas, um -20 dB sehen. Wenn du sprichst, sollte dies auf irgendwo zwischen -5 und -10 dB steigen.&lt;br /&gt;Wenn Du Echokompensation benutzt und es steigt auf mehr als -15 dB, wenn du nicht redest, funktioniert dein Setup nicht und du wirst andere Nutzer mit Echos zu ärgern.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Signal Analysis</source>
        <translation>Signalanalyse</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Microphone power</source>
        <translation>Mikrofonpegel</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>How close the current input level is to ideal</source>
        <translation>Wie nahe der gegenwärtige Eingangspegel am Ideal ist</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows how close your current input volume is to the ideal. To adjust your microphone level, open whatever program you use to adjust the recording volume, and look at the value here while talking.&lt;br /&gt;&lt;b&gt;Talk loud, as you would when you&apos;re upset over getting fragged by a noob.&lt;/b&gt;&lt;br /&gt;Adjust the volume until this value is close to 100%, but make sure it doesn&apos;t go above. If it does go above, you are likely to get clipping in parts of your speech, which will degrade sound quality.</source>
        <translation>Dies zeigt an, wie nahe deine gegenwärtige Eingangslautstärke am Idealwert liegt. Um deinen Mikrofonpegel anzupassen, öffne das Programm, das du für die Anpassung der Aufnahmelautstärke benutzt und sieh dir diese Werte an während du sprichst.&lt;br /&gt;&lt;b&gt;Sprich laut, so als würdest du dich darüber ärgern, von einem Anfänger aus dem Spiel geworfen zu werden.&lt;/b&gt;&lt;br /&gt;Passe die Lautstärke solange an, bis der Wert nahe bei 100% liegt. Stelle jedoch sicher, dass er nicht darüber hinaus geht. Sollte er darüber hinaus gehen, dann ist es wahrscheinlich, dass du Aussetzer in deiner Sprachübertragung bekommst, was die Klangqualität beeinträchtigt.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Signal-To-Noise ratio</source>
        <translation>Signal-zu-Rausch-Verhältnis</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal-To-Noise ratio from the microphone</source>
        <translation>Signal-zu-Rausch-Verhältnis vom Mikrofon</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the Signal-To-Noise Ratio (SNR) of the microphone in the last frame (20 ms). It shows how much clearer the voice is compared to the noise.&lt;br /&gt;If this value is below 1.0, there&apos;s more noise than voice in the signal, and so quality is reduced.&lt;br /&gt;There is no upper limit to this value, but don&apos;t expect to see much above 40-50 without a sound studio.</source>
        <translation>Dies ist das Signal-Rausch-Verhältnis (SRV) des Mikrofons im letzten Frame (20 ms). Es zeigt, wie viel klarer die Stimme im Vergleich zum Rauschen ist.&lt;br /&gt;Wenn dieser Wert unter 1,0 liegt, gibt es mehr Rauschen als Stimme im Signal und somit ist die Qualität vermindert.&lt;br /&gt;Es gibt keine Obergrenze für diesen Wert, aber oberhalb von 40-50 solltest du nicht erwarten viel zu sehen, ohne ein Tonstudio einzusetzen.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Probability</source>
        <translation>Sprachwahrscheinlichkeit</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Probability of speech</source>
        <translation>Wahrscheinlichkeit von Sprache</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the probability that the last frame (20 ms) was speech and not environment noise.&lt;br /&gt;Voice activity transmission depends on this being right. The trick with this is that the middle of a sentence is always detected as speech; the problem is the pauses between words and the start of speech. It&apos;s hard to distinguish a sigh from a word starting with &apos;h&apos;.&lt;br /&gt;If this is in bold font, it means RetroShare is currently transmitting (if you&apos;re connected).</source>
        <translation>Dies ist die Wahrscheinlichkeit, dass der letzte Frame (20 ms) Sprache und nicht Umgebungsrauschen war.&lt;br /&gt;Sprechpausenerkennung hängt davon ab, dass dies korrekt ist. Der Trick dabei ist, dass die Mitte eines Satzes immer als Sprache erkannt wird. Das Problem sind die Pausen zwischen den Wörtern und dem Beginn des Sprechens. Es ist schwer, einen Seufzer von einem Wort beginnend mit &apos;h&apos; zu unterscheiden.&lt;br /&gt;Wenn dies in Fettschrift erscheint, bedeutet es, dass Retroshare gerade überträgt (sofern du verbunden bist).</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Configuration feedback</source>
        <translation>Konfigurationsrückmeldung</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Current audio bitrate</source>
        <translation>Aktuelle Audiobitrate</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Bitrate of last frame</source>
        <translation>Bitrate des letzten Frames</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the audio bitrate of the last compressed frame (20 ms), and as such will jump up and down as the VBR adjusts the quality. The peak bitrate can be adjusted in the Settings dialog.</source>
        <translation>Dies ist die Audiobitrate des letzten komprimierten Frames (20 ms), und als solche wird sie auf und ab springen, während die VBR die Qualität anpasst. Der Spitzenwert kann im Einstellungsdialog angepasst werden.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>DoublePush interval</source>
        <translation>Doppeldrückinterval</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Time between last two Push-To-Talk presses</source>
        <translation>Zeit zwischen den letzten beiden Push-to-Talk-Klicks</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Detection</source>
        <translation>Spracherkennung</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Current speech detection chance</source>
        <translation>Aktuelle Spracherkennungschance</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This shows the current speech detection settings.&lt;/b&gt;&lt;br /&gt;You can change the settings from the Settings dialog or from the Audio Wizard.</source>
        <translation>&lt;b&gt;Dies zeigt die aktuellen Spracherkennungseinstellungen.&lt;/b&gt;&lt;br /&gt;Du kannst die Einstellungen im Einstellungsdialog oder im Audioassistenten ändern.</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Signal and noise power spectrum</source>
        <translation>Signal-zu-Rausch-Leistungsspektrum</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Power spectrum of input signal and noise estimate</source>
        <translation>Leistungsspektrum des Eingangssignals und Rauschschätzung</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the power spectrum of the current input signal (red line) and the current noise estimate (filled blue).&lt;br /&gt;All amplitudes are multiplied by 30 to show the interesting parts (how much more signal than noise is present in each waveband).&lt;br /&gt;This is probably only of interest if you&apos;re trying to fine-tune noise conditions on your microphone. Under good conditions, there should be just a tiny flutter of blue at the bottom. If the blue is more than halfway up on the graph, you have a seriously noisy environment.</source>
        <translation>Dies zeigt das Leistungsspektrum des aktuellen Eingangssignals (rote Linie) und die aktuelle Rauschschätzung (blau).&lt;br /&gt;Alle Amplituden werden mit 30 multipliziert, um die interessanten Teile (wie viel mehr Signal als Rauschen im jeweiligen Wellenband vorhanden ist) zu zeigen.&lt;br /&gt;Dies ist wahrscheinlich nur dann von Interesse, wenn du die Feinabstimmung deines Mikrofons auf die Geräuschkulisse versuchst. Unter guten Bedingungen, sollte nur ein winziges blaues Flattern am Boden zu sehen sein. Wenn das Blau mehr als die Hälfte nach oben geht, hast du eine wirklich laute Umgebung.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Echo Analysis</source>
        <translation>Echoanalyse</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Weights of the echo canceller</source>
        <translation>Wichtung des Echokompensators</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the weights of the echo canceller, with time increasing downwards and frequency increasing to the right.&lt;br /&gt;Ideally, this should be black, indicating no echo exists at all. More commonly, you&apos;ll have one or more horizontal stripes of bluish color representing time delayed echo. You should be able to see the weights updated in real time.&lt;br /&gt;Please note that as long as you have nothing to echo off, you won&apos;t see much useful data here. Play some music and things should stabilize. &lt;br /&gt;You can choose to view the real or imaginary parts of the frequency-domain weights, or alternately the computed modulus and phase. The most useful of these will likely be modulus, which is the amplitude of the echo, and shows you how much of the outgoing signal is being removed at that time step. The other viewing modes are mostly useful to people who want to tune the echo cancellation algorithms.&lt;br /&gt;Please note: If the entire image fluctuates massively while in modulus mode, the echo canceller fails to find any correlation whatsoever between the two input sources (speakers and microphone). Either you have a very long delay on the echo, or one of the input sources is configured wrong.</source>
        <translation>Dies zeigt die Wichtung des Echokompensators mit Zeit nach unten zunehmend und Frequenz nach rechts zunehmend.&lt;br /&gt;Idealerweise sollte dies schwarz sein und zeigen dass überhaupt kein Echo existiert. Für gewöhnlich wirst du ein oder mehrere horizontale bläuliche Streifen haben, die zeitverzögertes Echo darstellen. Du solltest in der Lage sein, die Wichtung in Echtzeit aktualisiert zu sehen.&lt;br /&gt;Bitte beachte, dass, solange du nichts hast was ein Echo erzeugt, du hier nicht viel nützliche Daten sehen wirst. Spiele etwas Musik ab und die Dinge sollten sich stabilisieren.&lt;br /&gt;Du kannst wählen ob du die realen oder imaginären Teile der Frequenzbereichswichtung oder alternativ die berechnete Betragsfunktion und Phase ansehen willst. Die nützlichste davon wird wahrscheinlich die Betragsfunktion sein, welche die Amplitude des Echos darstellt, und dir zeigt, wie viel von dem abgehenden Signal zu diesem Zeitpunkt entfernt wird. Die anderen Anzeigemodi sind für alle diejenigen nützlich, die die Algorithmen des Echokompensators abstimmen wollen.&lt;br /&gt;Bitte beachte: Wenn die gesamte Anzeige im Betragsfunktionsmodus massiv schwankt, dann findet der Echokompensator keinerlei Korrelation zwischen den beiden Signalquellen (Lautsprecher und Mikrofon). Entweder Sie haben eine sehr lange Verzögerung im Echo, oder eine der Eingangsquellen ist falsch konfiguriert.</translation>
    </message>
</context>
<context>
    <name>AudioWizard</name>
    <message>
        <location filename="../gui/AudioWizard.ui" line="+14"/>
        <source>Audio Tuning Wizard</source>
        <translation>Audioeinstellungsassistent</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Introduction</source>
        <translation>Einführung</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Welcome to the RetroShare Audio Wizard</source>
        <translation>Willkommen zum RetroShare-Audioassistenten</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This is the audio tuning wizard for RetroShare. This will help you correctly set the input levels of your sound card, and also set the correct parameters for sound processing in Retroshare. </source>
        <translation>Dies ist RetroShare Assistent zum konfigurieren Ihrer Audio-Einstellungen. Er wird Ihnen helfen die korrekte Eingangslautstärke Ihrer Soundkarte und die korrekten Parameter für die Tonverarbeitung in RetroShare zu wählen.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Volume tuning</source>
        <translation>Lautstärkeeinstellung</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tuning microphone hardware volume to optimal settings.</source>
        <translation>Mikrofonhardware-Lautstärke auf optimalen Wert einstellen.</translation>
    </message>
    <message>
        <source>&lt;p &gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there&apos;s an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it&apos;s checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation type="vanished">Öffne die Lautstärkeeinstellungen und gehe zu den Aufnahmeeinstellungen. Versichere dich, dass das Mikrofon als aktives Eingabegerät mit maximaler Aufnahmelautstärke ausgewählt ist. Falls es eine Option &quot;Mikrofon Boost&quot; gibt, sollte diese aktiviert sein.

Sprich so laut, als wärst du verärgert oder aufgeregt. Verringere die Lautstärke in den Lautstärkeeinstellungen bis der Balken so weit wie möglich oben im blauen und grünen, aber nicht im roten Bereich ist, während du sprichst.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&lt;p&gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there&apos;s an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it&apos;s checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+11"/>
        <source>Talk normally, and adjust the slider below so that the bar moves into green when you talk, and doesn&apos;t go into the orange zone.</source>
        <translation>Sprich normal, und passe den Schieberegler unten an, so dass der Balken sich ins Grün bewegt, wenn du sprichst, und nicht in die orange Zone geht.</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Stop looping echo for this wizard</source>
        <translation>Echoschleifen mit diesem Assistenten unterdrücken.</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Apply some high contrast optimizations for visually impaired users</source>
        <translation>Auf hohen Kontrast optimierte Darstellung für sehbehinderte Benutzer verwenden</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Use high contrast graphics</source>
        <translation>Anzeigen mit hohem Kontrast verwenden</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Voice Activity Detection</source>
        <translation>Sprechpausenerkennung</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Letting RetroShare figure out when you&apos;re talking and when you&apos;re silent.</source>
        <translation>RetroShare herausfinden lassen, wann du sprichst und wann nicht.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This will help Retroshare figure out when you are talking. The first step is selecting which data value to use.</source>
        <translation>Dies wird RetroShare helfen herauszufinden, wann du sprichst. Der erste Schritt ist den zu benutzenden Datenwert auszuwählen.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Push To Talk:</source>
        <translation>Push-To-Talk:</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Voice Detection</source>
        <translation>Spracherkennung</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Next you need to adjust the following slider. The first few utterances you say should end up in the green area (definitive speech). While talking, you should stay inside the yellow (might be speech) and when you&apos;re not talking, everything should be in the red (definitively not speech).</source>
        <translation>Als nächstes musst du den folgenden Schieber anpassen. Die ersten paar Geräusche die du beim Sprechen machst sollten im grünen Bereich (definitv Sprache) landen. Während du sprichst solltest du im gelben Bereich (könnte Sprache sein) bleiben und wenn du nicht sprichst, sollte alles im roten Bereich (definitiv keine Sprache) bleiben.</translation>
    </message>
    <message>
        <location line="+64"/>
        <source>Continuous transmission</source>
        <translation>Kontinuierliche Übertragung</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Finished</source>
        <translation>Fertig</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Enjoy using RetroShare</source>
        <translation>Viel Spaß mit RetroShare</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Congratulations. You should now be ready to enjoy a richer sound experience with Retroshare.</source>
        <translation>Herzlichen Glückwunsch. Du solltest nun eine reichere Sounderfahrung mit Retroshare machen.</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="+128"/>
        <source>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</source>
        <translation>&lt;h3&gt;RetroShare VOIP Plug-in&lt;/h3&gt;&lt;br/&gt; * Beitragende: Cyril Soler, Josselin Jacquard&lt;br/&gt;</translation>
    </message>
    <message>
        <source>&lt;br/&gt;The VOIP plugin adds VOIP to the private chat window of RetroShare. to use it, proceed as follows:&lt;UL&gt;</source>
        <translation type="vanished">&lt;br/&gt;Das VOIP Plug-in ermöglicht VOIP Telefonie im privaten Chat Fenster von RetroShare. Um es zu benutzen gehe folgendermaßen vor:&lt;ul&gt;</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;li&gt; setup microphone levels using the configuration panel&lt;/li&gt;</source>
        <translation>&lt;li&gt;Mikrofonlautstärke in den Optionen einstellen&lt;/li&gt;</translation>
    </message>
    <message>
        <source>&lt;li&gt; check your microphone by looking at the VU-metters&lt;/li&gt;</source>
        <translation type="vanished">&lt;li&gt;VU-Meter anschauen, um Mikrofon überprüfen&lt;/li&gt;</translation>
    </message>
    <message>
        <location line="-1"/>
        <source>&lt;br/&gt;The VOIP plugin adds VOIP to the private chat window of RetroShare. To use it, proceed as follows:&lt;UL&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;li&gt; check your microphone by looking at the VU-meters&lt;/li&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&lt;li&gt; in the private chat, enable sound input/output by clicking on the two VOIP icons&lt;/li&gt;&lt;/ul&gt;</source>
        <translation>&lt;li&gt;Ton Ein-/Ausgabe im privaten Chat Fenster aktivieren, indem du auf die zwei VOIP Icons klickst&lt;/li&gt;&lt;/ul&gt;</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Your friend needs to run the plugin to talk/listen to you, or course.</source>
        <translation>Dein Freund braucht natürlich auch das Plug-in um mit dir zu telefonieren.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&lt;br/&gt;&lt;br/&gt;This is an experimental feature. Don&apos;t hesitate to send comments and suggestion to the RS dev team.</source>
        <translation>&lt;br/&gt;&lt;br/&gt;Dies ist eine experimentelles Funktion. Zögere nicht Anmerkungen und Vorschläge an das RS Dev Team zu schicken.</translation>
    </message>
</context>
<context>
    <name>VOIP</name>
    <message>
        <location line="+47"/>
        <source>This plugin provides voice communication between friends in RetroShare.</source>
        <translation>Dieses Plug-in bietet Sprachkommunikation zwischen Freunden in RetroShare.</translation>
    </message>
    <message>
        <location line="+40"/>
        <location line="+4"/>
        <location line="+4"/>
        <location line="+4"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
    <message>
        <location line="-11"/>
        <source>Incoming audio call</source>
        <translation>Eingehender Anruf</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Incoming video call</source>
        <translation>Eingehender Videoanruf</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing audio call</source>
        <translation>Ausgehender Audioanruf</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing video call</source>
        <translation>Ausgehender Videoanruf</translation>
    </message>
</context>
<context>
    <name>VOIPChatWidgetHolder</name>
    <message>
        <location filename="../gui/VOIPChatWidgetHolder.cpp" line="+70"/>
        <location line="+146"/>
        <source>Mute</source>
        <translation>Stumm</translation>
    </message>
    <message>
        <location line="-128"/>
        <location line="+138"/>
        <source>Start Call</source>
        <translation>Anruf beginnen</translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Start Video Call</source>
        <translation>Videoanruf beginnen</translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Hangup Call</source>
        <translation>Anruf beenden</translation>
    </message>
    <message>
        <location line="-113"/>
        <location line="+626"/>
        <source>Hide Chat Text</source>
        <translation>Chattext ausblenden</translation>
    </message>
    <message>
        <location line="-608"/>
        <location line="+106"/>
        <location line="+523"/>
        <source>Fullscreen mode</source>
        <translation>Vollbildmodus</translation>
    </message>
    <message>
        <location line="-410"/>
        <source>Accept Audio Call</source>
        <translation>Audioanruf annehmen</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Decline Audio Call</source>
        <translation>Audioanruf ablehnen</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse audio call</source>
        <translation>Audioanruf abweisen</translation>
    </message>
    <message>
        <location line="+52"/>
        <source>Decline Video Call</source>
        <translation>Videoanruf ablehnen</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse video call</source>
        <translation>Videoanruf abweisen</translation>
    </message>
    <message>
        <location line="+102"/>
        <source>Mute yourself</source>
        <translation>Dich selbst stummschalten</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Unmute yourself</source>
        <translation>Deine Stummschaltung aufheben</translation>
    </message>
    <message>
        <location line="+603"/>
        <source>Your friend is calling you for video. Respond.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-781"/>
        <location line="+53"/>
        <location line="+188"/>
        <location line="+24"/>
        <location line="+57"/>
        <location line="+28"/>
        <location line="+284"/>
        <location line="+11"/>
        <location line="+11"/>
        <location line="+21"/>
        <location line="+11"/>
        <source>VoIP Status</source>
        <translation>VoIP-Status</translation>
    </message>
    <message>
        <location line="-467"/>
        <source>Hold Call</source>
        <translation>Anruf halten</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Outgoing Call is started...</source>
        <translation>Ausgehender Anruf hat begonnen...</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Resume Call</source>
        <translation>Anruf fortsetzen</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Outgoing Audio Call stopped.</source>
        <translation>Ausgehender Audioanruf wurde gestoppt.</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Shut camera off</source>
        <translation>Kamera ausschalten</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>You&apos;re now sending video...</source>
        <translation>Sie senden jetzt das Video...</translation>
    </message>
    <message>
        <location line="-266"/>
        <location line="+279"/>
        <source>Activate camera</source>
        <translation>Kamera aktivieren</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video call stopped</source>
        <translation>Videoanruf beendet</translation>
    </message>
    <message>
        <location line="-295"/>
        <source>Accept Video Call</source>
        <translation>Videoanruf annehmen</translation>
    </message>
    <message>
        <location line="-55"/>
        <source>%1 is inviting you to start an audio conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Activate audio</source>
        <translation>Audio aktivieren</translation>
    </message>
    <message>
        <location line="+50"/>
        <source>%1 is inviting you to start a video conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+334"/>
        <source>Show Chat Text</source>
        <translation>Chattext anzeigen</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Return to normal view.</source>
        <translation>Zurück zur normalen Ansicht.</translation>
    </message>
    <message>
        <location line="+228"/>
        <source>%1 hang up. Your call is closed.</source>
        <translation>%1 hat aufgelegt. Ihr Anruf ist geschlossen.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your audio call is closed.</source>
        <translation>%1 hat aufgelegt. Ihr Audioanruf ist geschlossen.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your video call is closed.</source>
        <translation>%1 hat aufgelegt. Ihr Videoanruf ist geschlossen.</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>%1 accepted your audio call.</source>
        <translation>%1 hat Ihren Audioanruf angenommen.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 accepted your video call.</source>
        <translation>%1 hat Ihren Videoanruf angenommen.</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Waiting for your friend to respond to your audio call.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+58"/>
        <source>Waiting for your friend to respond to your video call.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="-44"/>
        <source>Your friend is calling you for audio. Respond.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+17"/>
        <location line="+58"/>
        <source>Answer</source>
        <translation>Antworten</translation>
    </message>
</context>
<context>
    <name>VOIPPlugin</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="-48"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
</context>
<context>
    <name>VOIPToasterItem</name>
    <message>
        <location filename="../gui/VOIPToasterItem.cpp" line="+42"/>
        <source>Answer</source>
        <translation>Antworten</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Answer with video</source>
        <translation>Mit Video antworten</translation>
    </message>
    <message>
        <location filename="../gui/VOIPToasterItem.ui" line="+232"/>
        <source>Decline</source>
        <translation>Ablehnen</translation>
    </message>
</context>
<context>
    <name>VOIPToasterNotify</name>
    <message>
        <location filename="../gui/VOIPToasterNotify.cpp" line="+55"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Accept</source>
        <translation>Annehmen</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bandwidth Information</source>
        <translation>Bandbreiteninformation</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Audio or Video Data</source>
        <translation>Audio- oder Videodaten</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>HangUp</source>
        <translation>Auflegen</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Invitation</source>
        <translation>Einladung</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Audio Call</source>
        <translation>Audioanruf</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Video Call</source>
        <translation>Videoanruf</translation>
    </message>
    <message>
        <location line="+110"/>
        <source>Test VOIP Accept</source>
        <translation>VOIP-Annahme testen</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP BandwidthInfo</source>
        <translation>VOIP-Bandbreiteninformation testen</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Data</source>
        <translation>VOIP-Daten testen</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP HangUp</source>
        <translation>VOIP-Auflegen testen</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Invitation</source>
        <translation>VOIP-Einladung testen</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Test VOIP Audio Call</source>
        <translation>VOIP-Audioanruf testen</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Video Call</source>
        <translation>VOIP-Videoanruf testen</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Accept received from this peer.</source>
        <translation>Nachbar hat Einladung angenommen.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Bandwidth Info received from this peer: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bandwidth Info received from this peer:%1</source>
        <translation type="vanished">Bandbreiteninfos von diesem Nachbarn erhalten:%1</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Audio or Video Data received from this peer.</source>
        <translation>Audio- oder Videodaten von diesem Nachbarn erhalten.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>HangUp received from this peer.</source>
        <translation>Nachbar hat aufgelegt.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Invitation received from this peer.</source>
        <translation>Einladung von diesem Nachbarn erhalten.</translation>
    </message>
    <message>
        <location line="+25"/>
        <location line="+24"/>
        <source>calling</source>
        <translation>Anruf läuft</translation>
    </message>
</context>
<context>
    <name>voipGraphSource</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="-246"/>
        <source>Required bandwidth</source>
        <translation>Erforderliche Bandbreite</translation>
    </message>
</context>
</TS>
