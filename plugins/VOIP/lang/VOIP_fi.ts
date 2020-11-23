<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="fi">
<context>
    <name>AudioInput</name>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="+17"/>
        <source>Audio Wizard</source>
        <translation>Ohjattu toiminto ääniasetusten määrittämiseksi</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Transmission</source>
        <translation>Siirto</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&amp;Transmit</source>
        <translation>&amp;Siirrä</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>When to transmit your speech</source>
        <translation>Puheesi siirron määritys</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets when speech should be transmitted.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuous&lt;/i&gt; - All the time&lt;br /&gt;&lt;i&gt;Voice Activity&lt;/i&gt; - When you are speaking clearly.&lt;br /&gt;&lt;i&gt;Push To Talk&lt;/i&gt; - When you hold down the hotkey set under &lt;i&gt;Shortcuts&lt;/i&gt;.</source>
        <translation>&lt;b&gt;Määritä, milloin puhettasi siirretään.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Jatkuva&lt;/i&gt; - Koko ajan&lt;br /&gt;&lt;i&gt;Aktiivinen puhe&lt;/i&gt; - Kun puhut selvästi.&lt;br /&gt;&lt;i&gt;Paina puhuaksesi&lt;/i&gt; - Kun pidät pohjassa painiketta, jonka olet määrittänyt &lt;i&gt;pikanäppäimissä&lt;/i&gt;.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>DoublePush Time</source>
        <translation>Kaksoispainamisen aikajakso</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>If you press the PTT key twice in this time it will get locked.</source>
        <translation>Jos painat puhumisen pikanäppäintä kahdesti tänä aikana, se lukittuu.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;DoublePush Time&lt;/b&gt;&lt;br /&gt;If you press the push-to-talk key twice during the configured interval of time it will be locked. RetroShare will keep transmitting until you hit the key once more to unlock PTT again.</source>
        <translation>&lt;b&gt;Kaksoispainamisen aikajakso&lt;/b&gt;&lt;br /&gt;Jos painat puhumisen pikanäppäintä kahdesti määritetyn ajan puitteissa, se lukittuu. Retroshare jatkaa siirtoa, kunnes painat näppäintä vielä kerran, jolloin lukitus poistuu.</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Voice &amp;Hold</source>
        <translation>Äänen &amp;pito</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>How long to keep transmitting after silence</source>
        <translation>Kuinka kauan siirretään hiljaisuuden jälkeen</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This selects how long after a perceived stop in speech transmission should continue.&lt;/b&gt;&lt;br /&gt;Set this higher if your voice breaks up when you speak (seen by a rapidly blinking voice icon next to your name).</source>
        <translation>&lt;b&gt;Tämä valinta määrittää, kuinka kauan siirron tulisi jatkua havaitun puheen hiljentymisen jälkeen.&lt;/b&gt;&lt;br /&gt;Aseta arvo korkeammaksi, jos puheesi ei ole yhtenäistä (nopeasti vilkkuva äänikuvake nimesi vieressä ilmaisee katkonaisuutta).</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Silence Below</source>
        <translation>Hiljaisuus alla</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values below this count as silence</source>
        <translation>Signaaliarvot tämän alapuolella lasketaan hiljaisuudeksi</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+32"/>
        <source>&lt;b&gt;This sets the trigger values for voice detection.&lt;/b&gt;&lt;br /&gt;Use this together with the Audio Statistics window to manually tune the trigger values for detecting speech. Input values below &quot;Silence Below&quot; always count as silence. Values above &quot;Speech Above&quot; always count as voice. Values in between will count as voice if you&apos;re already talking, but will not trigger a new detection.</source>
        <translation>&lt;b&gt;Tämä asettaa raja-arvot puheentunnistukselle.&lt;/b&gt;&lt;br /&gt;Käytä tätä yhdessä Äänitilastot-ikkunan kanssa säätääksesi puheentunnistuksen raja-arvot käsin. Hiljaisuus alla -asetuksen alittavat arvot lasketaan aina hiljaisuudeksi. Puhe yllä -asetuksen ylittävät arvot lasketaan aina puheeksi. Niiden väliin sijoittuvat arvot lasketaan puheeksi, jos puhut jo valmiiksi, mutta ne eivät laukaise uutta tunnistusta.</translation>
    </message>
    <message>
        <location line="-10"/>
        <source>Speech Above</source>
        <translation>Puhe yllä</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values above this count as voice</source>
        <translation>Signaaliarvot tämän yläpuolella lasketaan puheeksi</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>empty</source>
        <translation>tyhjä</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Audio Processing</source>
        <translation>Äänen käsittely</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Noise Suppression</source>
        <translation>Kohinanpoisto</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Noise suppression</source>
        <translation>Kohinanpoisto</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets the amount of noise suppression to apply.&lt;/b&gt;&lt;br /&gt;The higher this value, the more aggressively stationary noise will be suppressed.</source>
        <translation>&lt;b&gt;Tämä määrittää kohinanpoiston voimakkuuden.&lt;/b&gt;&lt;br /&gt;Mitä korkeampi arvo, sitä aggressiivisemmin kohinaa poistetaan.</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Amplification</source>
        <translation>Vahvistus</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Maximum amplification of input sound</source>
        <translation>Äänen suurin vahvistus</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;Maximum amplification of input.&lt;/b&gt;&lt;br /&gt;RetroShare normalizes the input volume before compressing, and this sets how much it&apos;s allowed to amplify.&lt;br /&gt;The actual level is continually updated based on your current speech pattern, but it will never go above the level specified here.&lt;br /&gt;If the &lt;i&gt;Microphone loudness&lt;/i&gt; level of the audio statistics hover around 100%, you probably want to set this to 2.0 or so, but if, like most people, you are unable to reach 100%, set this to something much higher.&lt;br /&gt;Ideally, set it so &lt;i&gt;Microphone Loudness * Amplification Factor &gt;= 100&lt;/i&gt;, even when you&apos;re speaking really soft.&lt;br /&gt;&lt;br /&gt;Note that there is no harm in setting this to maximum, but RetroShare will start picking up other conversations if you leave it to auto-tune to that level.</source>
        <translation>&lt;b&gt;Äänen suurin vahvistus.&lt;/b&gt;&lt;br /&gt;Retroshare normalisoi äänenvoimakkuuden ennen kompressointia ja tämä määrittää, kuinka suuri vahvistus sallitaan.&lt;br /&gt;Tosiasiallista tasoa päivitetään jatkuvasti puhemallin mukaan, mutta se ei koskaan ylitä tässä määritettyä tasoa.&lt;br /&gt;Jos &lt;i&gt;Mikrofonin voimakkuuden&lt;/i&gt; taso äänitilastoissa on lähellä 100%, kannattaa tämä asettaa arvoon 2.0, mutta jos, kuten tavallista, et pääse 100% asti, aseta arvoksi paljon suurempi.&lt;br /&gt;Paras asetus on &lt;i&gt;Mikrofonin voimakkuus * Vahvistuskerrois &gt;= 100&lt;/i&gt;, vaikka puhuisit hyvin pehmeästi.&lt;br /&gt;&lt;br /&gt;Tämän asettamisesta maksimiarvoon ei ole haittaa, mutta Retroshare alkaa poimia muita keskusteluja, jos jätät sen automaattisesti asettumaan maksimimitasoon.</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Echo Cancellation Processing</source>
        <translation>Kaiunesto</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video Processing</source>
        <translation>Videon käsittely</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Available bandwidth:</source>
        <translation>Kaistaa käytettävissä:</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Use this field to simulate the maximum bandwidth available so as to preview what the encoded video will look like with the corresponding compression rate.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Käytä tätä kenttää simuloidaksesi suurinta käytettävissä olevaa kaistanleveyttä. Saat näin esimakua siitä, miltä koodattu video näyttää vastaavalla pakkaussuhteella.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>KB/s</source>
        <translation>kt/s</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Display encoded (and then decoded) frame, to check the codec&apos;s quality. If not selected, the image above only shows the frame that is grabbed from your camera.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Näytä koodattu (ja sitten dekoodattu) kuva tarkistaaksesi koodekin laadun. Jos tätä ei ole valittu, yllä oleva kuva näyttää vain kamerastasi kaapatun kuvan.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>preview</source>
        <translation>esikatselu</translation>
    </message>
</context>
<context>
    <name>AudioInputConfig</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="+202"/>
        <source>Continuous</source>
        <translation>Jatkuva</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Voice Activity</source>
        <translation>Aktiivinen puhe</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Push To Talk</source>
        <translation>Paina puhuaksesi</translation>
    </message>
    <message>
        <location line="+105"/>
        <source>%1 s</source>
        <translation>%1 s</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Off</source>
        <translation>Pois</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>-%1 dB</source>
        <translation>-%1 dB</translation>
    </message>
    <message>
        <location filename="../gui/AudioInputConfig.h" line="+94"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
</context>
<context>
    <name>AudioStats</name>
    <message>
        <location filename="../gui/AudioStats.ui" line="+14"/>
        <source>Audio Statistics</source>
        <translation>Äänitilastot</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Input Levels</source>
        <translation>Äänitasot</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Peak microphone level</source>
        <translation>Mikrofonin huipputaso</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+20"/>
        <location line="+20"/>
        <source>Peak power in last frame</source>
        <translation>Viimeisen ääniviipaleen huipputeho</translation>
    </message>
    <message>
        <location line="-37"/>
        <source>This shows the peak power in the last frame (20 ms), and is the same measurement as you would usually find displayed as &quot;input power&quot;. Please disregard this and look at &lt;b&gt;Microphone power&lt;/b&gt; instead, which is much more steady and disregards outliers.</source>
        <translation>Tämä näyttää huipputehon viimeisessä ääniviipaleessa (20 ms). Tämä on sama mittaus, joka yleensä esitettäisiin &quot;tulotehona&quot;. Älä huomioi tätä, vaan katso mielummin &lt;b&gt;Mikrofonin tehoa&lt;/b&gt;, joka on paljon vakaampi ja jättää häiriöt huomiotta.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak speaker level</source>
        <translation>Kaiuttimen huipputaso</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power of the speakers in the last frame (20 ms). Unless you are using a multi-channel sampling method (such as ASIO) with speaker channels configured, this will be 0. If you have such a setup configured, and this still shows 0 while you&apos;re playing audio from other programs, your setup is not working.</source>
        <translation>Näyttää kaiuttimien voimakkuuden huipputason viimeisessä kuvassa (20ms). Jos et käytä monikanavaista otosmenetelmää (kuten ASIO) kaiuttimien kanavien ollessa säädetyt, tämä on 0.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak clean level</source>
        <translation>Puhtaantason huippu</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power in the last frame (20 ms) after all processing. Ideally, this should be -96 dB when you&apos;re not talking. In reality, a sound studio should see -60 dB, and you should hopefully see somewhere around -20 dB. When you are talking, this should rise to somewhere between -5 and -10 dB.&lt;br /&gt;If you are using echo cancellation, and this rises to more than -15 dB when you&apos;re not talking, your setup is not working, and you&apos;ll annoy other users with echoes.</source>
        <translation>Tämä näyttää äänenvoimakkuuden huipputason edellisessä kuvassa (20 ms) käsittelyn jälkeen. Tämän tulisi olla ihanteellisesti -96 dB, kun et puhu. Todellisuudessa taso on äänitysstudiossa -60 dB ja voit olla tyytyväinen, jos itse näet arvona noin -20 dB. Kun puhut, taso nousee -5 ja -10 dB:n välille.&lt;br /&gt;Jos käytät kaiunpoistoa ja taso nousee yli -15 dB ollessasi hiljaa, säätösi eivät toimi ja ärsytät kaiullasi muita kuulijoita.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Signal Analysis</source>
        <translation>Signaalianalyysi</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Microphone power</source>
        <translation>Mikrofonin voimakkuus</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>How close the current input level is to ideal</source>
        <translation>Kuinka lähellä nykyinen syöttötaso on ihannetta</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows how close your current input volume is to the ideal. To adjust your microphone level, open whatever program you use to adjust the recording volume, and look at the value here while talking.&lt;br /&gt;&lt;b&gt;Talk loud, as you would when you&apos;re upset over getting fragged by a noob.&lt;/b&gt;&lt;br /&gt;Adjust the volume until this value is close to 100%, but make sure it doesn&apos;t go above. If it does go above, you are likely to get clipping in parts of your speech, which will degrade sound quality.</source>
        <translation>Tämä näyttää, kuinka lähellä ihanteellista nykyinen sisääntulevan äänen taso on. Säätääksesi mikrofonisi tasoa, avaa käyttöjärjestelmäsi ääniasetukset ja tarkkaile tallennuslaitteen tasoa puhuessasi.&lt;br /&gt;&lt;b&gt;Puhu kovaa, ikään kuin olisit ärtynyt.&lt;/b&gt;&lt;br /&gt;Säädä mikrofonin äänenvoimakkuuden tasoa, kunnes tämä arvo on lähellä 100%, mutta varmista ettei se ylity. Arvon ylittyessä puheessasi ilmenee pätkimistä, mikä heikentää äänenlaatua.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Signal-To-Noise ratio</source>
        <translation>Signaali-kohinasuhde</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal-To-Noise ratio from the microphone</source>
        <translation>Signaali-kohinasuhde mikrofonista</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the Signal-To-Noise Ratio (SNR) of the microphone in the last frame (20 ms). It shows how much clearer the voice is compared to the noise.&lt;br /&gt;If this value is below 1.0, there&apos;s more noise than voice in the signal, and so quality is reduced.&lt;br /&gt;There is no upper limit to this value, but don&apos;t expect to see much above 40-50 without a sound studio.</source>
        <translation>Näyttää viimeisimmän paketin (20ms) mikrofonin Signal-To-Noise -suhteen (SNR). Arvosta näkee kuinka paljon selkeämpi ääni on verrattuna meluun.&lt;br /&gt;Arvon ollessa alle 1.0, signaalissa on enemmän melua kuin ääntä, jolloin laatu on heikkoa.&lt;br /&gt;Arvolle ei ole ylärajaa, mutta älä oleta saavasi 40-50 ylittäviä arvoja ilman äänitysstudiota.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Probability</source>
        <translation>Puheen todennäköisyys</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Probability of speech</source>
        <translation>Puheen todennäköisyys</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the probability that the last frame (20 ms) was speech and not environment noise.&lt;br /&gt;Voice activity transmission depends on this being right. The trick with this is that the middle of a sentence is always detected as speech; the problem is the pauses between words and the start of speech. It&apos;s hard to distinguish a sigh from a word starting with &apos;h&apos;.&lt;br /&gt;If this is in bold font, it means RetroShare is currently transmitting (if you&apos;re connected).</source>
        <translation>Todennäköisyys sille, että viimeisin paketti (20ms) oli puhetta eikä taustamelua.&lt;br /&gt;Puheaktiviteetin lähetystapa riippuu siitä että tämä oikein. Lauseen keskiosa tunnistetaan aina puheeksi; ongelmat ovat lyhyet katkot sanojen keskellä ja puheen aloitus. Esimerkiksi huokaus on hankala erottaa h-kirjaimella alkavasta sanasta.&lt;br /&gt;Jos arvon kirjasin on lihavoitu, RetroShare lähettää ääntä paraikaa (jos olet yhteydessä).</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Configuration feedback</source>
        <translation>Asetusten palaute</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Current audio bitrate</source>
        <translation>Nykyinen äänen bittinopeus</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Bitrate of last frame</source>
        <translation>Viimeisen kuvan bittinopeus</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the audio bitrate of the last compressed frame (20 ms), and as such will jump up and down as the VBR adjusts the quality. The peak bitrate can be adjusted in the Settings dialog.</source>
        <translation>Viimeisimmän paketin (20ms) audion bittivirta. Arvo heiluu ylös ja alas aina kun VBR säätää laatua. Huipun bittivirtaa voidaan säätää Asetuksista.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>DoublePush interval</source>
        <translation>Tuplapainalluksen väli</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Time between last two Push-To-Talk presses</source>
        <translation>Viimeisimmän kahden puhepikanäppäimen painalluksen välinen aika</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Detection</source>
        <translation>Puheen havaitseminen</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Current speech detection chance</source>
        <translation>Nykyinen puheen havaitsemisen todennäköisyys</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This shows the current speech detection settings.&lt;/b&gt;&lt;br /&gt;You can change the settings from the Settings dialog or from the Audio Wizard.</source>
        <translation>&lt;b&gt;Näyttää nykyisen puheentunnistuksen asetukset.&lt;/b&gt;&lt;br /&gt;Voit vaihtaa asetusta Asetuksista tai ohjatusta asetustoiminnosta.</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Signal and noise power spectrum</source>
        <translation>Signaalin ja melun voimakkuuden jakauma</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Power spectrum of input signal and noise estimate</source>
        <translation>Sisääntulevan signaalin ja melun arvion jakauma.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the power spectrum of the current input signal (red line) and the current noise estimate (filled blue).&lt;br /&gt;All amplitudes are multiplied by 30 to show the interesting parts (how much more signal than noise is present in each waveband).&lt;br /&gt;This is probably only of interest if you&apos;re trying to fine-tune noise conditions on your microphone. Under good conditions, there should be just a tiny flutter of blue at the bottom. If the blue is more than halfway up on the graph, you have a seriously noisy environment.</source>
        <translation>Näyttää sisääntulevan signaalin (punainen) ja nykyisen meluarvion (sininen) voimakkuuden jakauman.&lt;br /&gt;Kaikki voimakkuudet ovat kerrottu 30:llä, jotta mielenkiintoiset osat näkyisivät (kuinka paljon enemmän ääntä kuin melua on jokaisessa ääniaallossa).&lt;br /&gt;Tämä kiinnostanee sinua vain jos yrität hienosäätää mikrofonin meluolosuhteita. Hyvissä olosuhteissa tulisi ilmetä vain pieni määrä sinistä alareunassa. Jos sinistä on enemmän kuin kuvaajan puoliväliin, ympäristösi on todella meluisa.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Echo Analysis</source>
        <translation>Kaiun analysointi</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Weights of the echo canceller</source>
        <translation>Kaiunpoiston painotukset</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the weights of the echo canceller, with time increasing downwards and frequency increasing to the right.&lt;br /&gt;Ideally, this should be black, indicating no echo exists at all. More commonly, you&apos;ll have one or more horizontal stripes of bluish color representing time delayed echo. You should be able to see the weights updated in real time.&lt;br /&gt;Please note that as long as you have nothing to echo off, you won&apos;t see much useful data here. Play some music and things should stabilize. &lt;br /&gt;You can choose to view the real or imaginary parts of the frequency-domain weights, or alternately the computed modulus and phase. The most useful of these will likely be modulus, which is the amplitude of the echo, and shows you how much of the outgoing signal is being removed at that time step. The other viewing modes are mostly useful to people who want to tune the echo cancellation algorithms.&lt;br /&gt;Please note: If the entire image fluctuates massively while in modulus mode, the echo canceller fails to find any correlation whatsoever between the two input sources (speakers and microphone). Either you have a very long delay on the echo, or one of the input sources is configured wrong.</source>
        <translation>Näyttää kaiunpoiston painotukset, aika lisääntyy alaspäin ja taajuus kasvaa oikealle.&lt;br /&gt;Ihanteellisesti tämä olisi musta, jolloin kaikua ei ole lainkaan. Useimmiten kaikua on useamman vaakasuuntaisen viivan verran. Viivat edustavat kaiun viivettä. Muutoksien tulisi näkyä reaaliajassa.&lt;br /&gt;Huomioi, että kun kaikua ei ole, tästä ei erityisemmin ilmene mitään. Soita hieman musiikkia, niin tilanne tasaantuu.&lt;br /&gt;Voit valita näytettäväksi joko oikean tai kuvitteelliset osat taajuuksien painotuksesta, tai vaihtoehtoisesti lasketun kertoimen tai näkymän. Näistä hyödyllisin lienee kerroin, joka on kaiun voimakkuus, ja näyttää paljonko uloslähtevästä signaalista leikataan. Muut näkymät ovat hyödyllisiä pääasiassa vain niille, jotka haluavat säätää kaiunpoiston algoritmeja.&lt;br /&gt;Huomioi, että jos koko kuva aaltoilee kerrointilassa, kaiunpoisto ei löydä yhteyttä kahden sisääntulolähteen välillä (kaiuttimien ja mikrofonin). Joko sinulla on pitkäviiveinen kaiku tai toinen sisääntulolähteistä on asetettu virheellisesti.</translation>
    </message>
</context>
<context>
    <name>AudioWizard</name>
    <message>
        <location filename="../gui/AudioWizard.ui" line="+14"/>
        <source>Audio Tuning Wizard</source>
        <translation>Ohjattu toiminto ääniasetusten määrittämiseksi</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Introduction</source>
        <translation>Johdatus</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Welcome to the RetroShare Audio Wizard</source>
        <translation>Tervetuloa RetroSharen ohjattuun ääniasetusten määrittamistoimintoon</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This is the audio tuning wizard for RetroShare. This will help you correctly set the input levels of your sound card, and also set the correct parameters for sound processing in Retroshare. </source>
        <translation>Tämä on Retrosharen ohjattu toiminto äänen säätämiseksi. Toiminto auttaa sinua asettamaan oikein äänikorttisi sisääntulon voimakkuustason ja oikeat parametrit Retrosharen äänenkäsittelylle.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Volume tuning</source>
        <translation>Äänitason viritys</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tuning microphone hardware volume to optimal settings.</source>
        <translation>Mikrofonin laiteäänenvoimakkuuden asettaminen sopivaksi.</translation>
    </message>
    <message>
        <source>&lt;p &gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there&apos;s an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it&apos;s checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation type="vanished">&lt;p&gt;Avaa tietokoneesi ääniasetukset ja mene tallennusasetuksiin. Varmista että mikrofoni on valittuna sisääntulona ja että sen nauhoitusvoimakkuus on maksimissa. Valitse mikrofonin voimakkuuden tehostus, jos se on mahdollista.&lt;/p&gt;
&lt;p&gt;Puhu kovalla äänellä, aivan kuin olisit ärsyyntynyt tai kiihtynyt. Vähennä ohjauspaneelissa äänenvoimakkuutta kunnes alapuolen palkki pysyy puhuessasi mahdollisimman korkealla vihreällä ja oranssilla alueella, mutta ei punaisella.&lt;/p&gt;</translation>
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
        <translation>Puhu tavallisella äänellä ja säädä alla olevaa liukua siten, että palkki liikkuu puhuessasi vihreään, eikä mene oranssille alueelle.</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Stop looping echo for this wizard</source>
        <translation>Pysäytä kaiun kiertäminen ohjatussa toiminnossa</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Apply some high contrast optimizations for visually impaired users</source>
        <translation>Käytä korkealle kontrastille optimoitua näkymää näkörajoitteisille käyttäjille.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Use high contrast graphics</source>
        <translation>Käytä korkean kontrastin grafiikoita</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Voice Activity Detection</source>
        <translation>Ääniaktiviteetin tunnistaminen</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Letting RetroShare figure out when you&apos;re talking and when you&apos;re silent.</source>
        <translation>Puheen ja hiljaisuuden tunnistuksen määrittäminen Retroshareen.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This will help Retroshare figure out when you are talking. The first step is selecting which data value to use.</source>
        <translation>Avustaa Retrosharea päättelemään, milloin olet äänessä. Ensimmäinen askel on menetelmän valinta.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Push To Talk:</source>
        <translation>Paina puhuaksesi</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Voice Detection</source>
        <translation>Puheen havaitseminen</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Next you need to adjust the following slider. The first few utterances you say should end up in the green area (definitive speech). While talking, you should stay inside the yellow (might be speech) and when you&apos;re not talking, everything should be in the red (definitively not speech).</source>
        <translation>Seuraavaksi sinun täytyy käyttää liukusäädintä. Aluksi puheesi tulisi päätyä vihreälle alueelle (varmasti puhetta). Kun jatkat puhumista, tulisi palkin pysyä keltaisella alueella (mahdollisesti puhetta). Kun olet hiljaa, tulisi palkin olla punaisella alueella (ei puhetta).</translation>
    </message>
    <message>
        <location line="+64"/>
        <source>Continuous transmission</source>
        <translation>Jatkuva siirto</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Finished</source>
        <translation>Valmis</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Enjoy using RetroShare</source>
        <translation>Nauti RetroSharesta</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Congratulations. You should now be ready to enjoy a richer sound experience with Retroshare.</source>
        <translation>Onneksi olkoon. Kaiken pitäisi nyt olla valmista täysipainoiselle äänikokemukselle Retrosharella</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="+128"/>
        <source>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</source>
        <translation>&lt;h3&gt;Retrosharen VOIP-lisäosa&lt;/h3&gt;&lt;br/&gt; * Avustajat: Cyril Soler, Josselin Jacquard&lt;br/&gt;</translation>
    </message>
    <message>
        <source>&lt;br/&gt;The VOIP plugin adds VOIP to the private chat window of RetroShare. to use it, proceed as follows:&lt;UL&gt;</source>
        <translation type="vanished">&lt;br/&gt;VOIP-lisäosa näkyy Retrosharen yksityisessä keskusteluikkunassa. Toimi seuraavasti, kun haluat käyttää sitä:&lt;UL&gt;</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;li&gt; setup microphone levels using the configuration panel&lt;/li&gt;</source>
        <translation>&lt;li&gt; aseta mikrofonin tasot käyttämällä asetusnäkymää&lt;/li&gt;</translation>
    </message>
    <message>
        <source>&lt;li&gt; check your microphone by looking at the VU-metters&lt;/li&gt;</source>
        <translation type="vanished">&lt;li&gt; tarkista mikrofonisi katsomalla äänitasomittaria&lt;/li&gt;</translation>
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
        <translation>&lt;li&gt; ota äänitulo ja -lähtö käyttöön painamalla keskusteluikkunan kahta VOIP-kuvaketta&lt;/li&gt;&lt;/ul&gt;</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Your friend needs to run the plugin to talk/listen to you, or course.</source>
        <translation>Ystäväsi on myös otettava lisäosa käyttöön puhuakseen kanssasi.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&lt;br/&gt;&lt;br/&gt;This is an experimental feature. Don&apos;t hesitate to send comments and suggestion to the RS dev team.</source>
        <translation>&lt;br/&gt;&lt;br/&gt;Tämä on kokeellinen ominaisuus. Lähetä kommentit ja parannusehdotukset Retrosharen kehitystiimille.</translation>
    </message>
</context>
<context>
    <name>VOIP</name>
    <message>
        <location line="+47"/>
        <source>This plugin provides voice communication between friends in RetroShare.</source>
        <translation>Tämän lisäosan avulla ystävät voivat olla puheyhteydessä Retrosharen kautta.</translation>
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
        <translation>Saapuva äänipuhelu</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Incoming video call</source>
        <translation>Saapuva videopuhelu</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing audio call</source>
        <translation>Lähtevä äänipuhelu</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing video call</source>
        <translation>Lähtevä videopuhelu</translation>
    </message>
</context>
<context>
    <name>VOIPChatWidgetHolder</name>
    <message>
        <location filename="../gui/VOIPChatWidgetHolder.cpp" line="+70"/>
        <location line="+146"/>
        <source>Mute</source>
        <translation>Mykistä</translation>
    </message>
    <message>
        <location line="-128"/>
        <location line="+138"/>
        <source>Start Call</source>
        <translation>Soita puhelu</translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Start Video Call</source>
        <translation>Soita videopuhelu</translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Hangup Call</source>
        <translation>Lopeta puhelu</translation>
    </message>
    <message>
        <location line="-113"/>
        <location line="+626"/>
        <source>Hide Chat Text</source>
        <translation>Piilota chat-teksti</translation>
    </message>
    <message>
        <location line="-608"/>
        <location line="+106"/>
        <location line="+523"/>
        <source>Fullscreen mode</source>
        <translation>Koko näyttö</translation>
    </message>
    <message>
        <source>%1 inviting you to start an audio conversation. Do you want Accept or Decline the invitation?</source>
        <translation type="vanished">%1 kutsuu sinua aloittamaan äänikeskustelun. Haluatko hyväksyä kutsun vai kieltäytyä?</translation>
    </message>
    <message>
        <location line="-410"/>
        <source>Accept Audio Call</source>
        <translation>Hyväksy äänipuhelu</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Decline Audio Call</source>
        <translation>Kieltäydy äänipuhelusta</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse audio call</source>
        <translation>Kieltäydy äänipuhelusta</translation>
    </message>
    <message>
        <source>%1 inviting you to start a video conversation. Do you want Accept or Decline the invitation?</source>
        <translation type="vanished">%1 kutsuu sinua aloittamaan videokeskustelun. Haluatko hyväksyä kutsun vai kieltäytyä?</translation>
    </message>
    <message>
        <location line="+52"/>
        <source>Decline Video Call</source>
        <translation>Kieltäydy videopuhelusta</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse video call</source>
        <translation>Kieltäydy videopuhelusta</translation>
    </message>
    <message>
        <location line="+102"/>
        <source>Mute yourself</source>
        <translation>Mykistä itsesi</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Unmute yourself</source>
        <translation>Mykistys pois itseltä</translation>
    </message>
    <message>
        <source>Waiting your friend respond your video call.</source>
        <translation type="vanished">Odotan, että ystäväsi vastaa videopuheluusi.</translation>
    </message>
    <message>
        <location line="+603"/>
        <source>Your friend is calling you for video. Respond.</source>
        <translation>Ystäväsi soittaa sinulle videolla. Vastaa.</translation>
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
        <translation>VoIP tila</translation>
    </message>
    <message>
        <location line="-467"/>
        <source>Hold Call</source>
        <translation>Pistä puhelu odotukseen</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Outgoing Call is started...</source>
        <translation>Lähtevä puhelu aloitettu...</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Resume Call</source>
        <translation>Palaa puheluun</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Outgoing Audio Call stopped.</source>
        <translation>Lähtevä äänipuhelu pysäytetty.</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Shut camera off</source>
        <translation>Sulje kamera</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>You&apos;re now sending video...</source>
        <translation>Lähetät nyt videota...</translation>
    </message>
    <message>
        <location line="-266"/>
        <location line="+279"/>
        <source>Activate camera</source>
        <translation>Ota kamera käyttöön</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video call stopped</source>
        <translation>Videopuhelu pysäytetty</translation>
    </message>
    <message>
        <location line="-295"/>
        <source>Accept Video Call</source>
        <translation>Hyväksy videopuhelu</translation>
    </message>
    <message>
        <location line="-55"/>
        <source>%1 is inviting you to start an audio conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Activate audio</source>
        <translation>Ota ääni käyttöön</translation>
    </message>
    <message>
        <location line="+50"/>
        <source>%1 is inviting you to start a video conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+334"/>
        <source>Show Chat Text</source>
        <translation>Näytä chat-teksti</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Return to normal view.</source>
        <translation>Palaa normalinäkymään.</translation>
    </message>
    <message>
        <location line="+228"/>
        <source>%1 hang up. Your call is closed.</source>
        <translation>%1 päätti puhelun. Puhelusi päättyi.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your audio call is closed.</source>
        <translation>%1 päätti puhelun. Äänipuhelusi päättyi.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your video call is closed.</source>
        <translation>%1 päätti puhelun. Videopuhelusi päättyi.</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>%1 accepted your audio call.</source>
        <translation>%1 hyväksyi äänipuhelusi.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 accepted your video call.</source>
        <translation>%1 hyväksyi videopuhelusi.</translation>
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
        <source>Waiting your friend respond your audio call.</source>
        <translation type="vanished">Odotan, että ystäväsi vastaa äänipuheluusi.</translation>
    </message>
    <message>
        <location line="-44"/>
        <source>Your friend is calling you for audio. Respond.</source>
        <translation>Ystäväsi soittaa sinulle äänellä. Vastaa.</translation>
    </message>
    <message>
        <location line="+17"/>
        <location line="+58"/>
        <source>Answer</source>
        <translation>Vastaa</translation>
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
        <location filename="../gui/VOIPToasterItem.cpp" line="+43"/>
        <source>Answer</source>
        <translation>Vastaa</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Answer with video</source>
        <translation>Vastaa videolla</translation>
    </message>
    <message>
        <location filename="../gui/VOIPToasterItem.ui" line="+232"/>
        <source>Decline</source>
        <translation>Kieltäydy</translation>
    </message>
</context>
<context>
    <name>VOIPToasterNotify</name>
    <message>
        <location filename="../gui/VOIPToasterNotify.cpp" line="+56"/>
        <source>VOIP</source>
        <translation>VOIP</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Accept</source>
        <translation>Hyväksy</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bandwidth Information</source>
        <translation>Kaistanleveystiedot</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Audio or Video Data</source>
        <translation>Ääni- tai Videodata</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>HangUp</source>
        <translation>Sulje puhelu</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Invitation</source>
        <translation>Kutsu</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Audio Call</source>
        <translation>Äänipuhelu</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Video Call</source>
        <translation>Videpuhelu</translation>
    </message>
    <message>
        <location line="+110"/>
        <source>Test VOIP Accept</source>
        <translation>Testaa VOIP:n hyväksymistä</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP BandwidthInfo</source>
        <translation>Testaa VOIP:n kaistainfoa</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Data</source>
        <translation>Testaa VOIP-dataa</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP HangUp</source>
        <translation>Testaa VOIP:n katkaisua</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Invitation</source>
        <translation>Testaa VOIP:n kutsumista</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Test VOIP Audio Call</source>
        <translation>Testaa VOIP:n äänipuhelua</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Video Call</source>
        <translation>Testaa VOIP:n videopuhelua</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Accept received from this peer.</source>
        <translation>Hyväksyntä saatu vertaiselta.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Bandwidth Info received from this peer: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bandwidth Info received from this peer:%1</source>
        <translation type="vanished">Kaistainfo vastaanotettu vertaiselta: %1</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Audio or Video Data received from this peer.</source>
        <translation>Ääni- tai videodata vastaanotettu vertaiselta.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>HangUp received from this peer.</source>
        <translation>Katkaisu vastaanotettu vertaiselta.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Invitation received from this peer.</source>
        <translation>Kutsu vastaanotettu vertaiselta.</translation>
    </message>
    <message>
        <location line="+25"/>
        <location line="+24"/>
        <source>calling</source>
        <translation>Soitan...</translation>
    </message>
</context>
<context>
    <name>voipGraphSource</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="-260"/>
        <source>Required bandwidth</source>
        <translation>Vaadittu kaistanleveys</translation>
    </message>
</context>
</TS>
