<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="fr">
<context>
    <name>AudioInput</name>
    <message>
        <location filename="../gui/AudioInputConfig.ui" line="+17"/>
        <source>Audio Wizard</source>
        <translation>Assistant audio</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Transmission</source>
        <translation>Transmission</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>&amp;Transmit</source>
        <translation>&amp;Transmission</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>When to transmit your speech</source>
        <translation>Quand transmettre votre voix</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets when speech should be transmitted.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continuous&lt;/i&gt; - All the time&lt;br /&gt;&lt;i&gt;Voice Activity&lt;/i&gt; - When you are speaking clearly.&lt;br /&gt;&lt;i&gt;Push To Talk&lt;/i&gt; - When you hold down the hotkey set under &lt;i&gt;Shortcuts&lt;/i&gt;.</source>
        <translation>&lt;b&gt;Détermine quand transmettre votre voix.&lt;/b&gt;&lt;br /&gt;&lt;i&gt;Continu&lt;/i&gt; - En continu&lt;br /&gt;&lt;i&gt;Activité Vocale&lt;/i&gt; - Quand vous parlez clairement.&lt;br /&gt;&lt;i&gt;Appuyez pour parler&lt;/i&gt; - Quand vous pressez la touche choisie dans les &lt;i&gt;Raccourcis&lt;/i&gt;.</translation>
    </message>
    <message>
        <location line="+14"/>
        <source>DoublePush Time</source>
        <translation>Temps entre deux pressions</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>If you press the PTT key twice in this time it will get locked.</source>
        <translation>Si vous pressez la touche Appuyez-pour-parler deux fois dans cet intervale de temps, cela la bloquera.</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;DoublePush Time&lt;/b&gt;&lt;br /&gt;If you press the push-to-talk key twice during the configured interval of time it will be locked. RetroShare will keep transmitting until you hit the key once more to unlock PTT again.</source>
        <translation>&lt;b&gt;Temps entre deux pressions&lt;/b&gt;&lt;br/&gt;Si vous pressez la touche Appuyez-pour-parler deux fois durant l&apos;intervale de temps configuré, cela la verrouillera. Retroshare continuera à transmettre tant que la touche ne sera pas enfoncée une nouvelle fois.</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Voice &amp;Hold</source>
        <translation>Maintient de la &amp;voix</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>How long to keep transmitting after silence</source>
        <translation>Combien de temps après un silence continuer à maintenir la transmission</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This selects how long after a perceived stop in speech transmission should continue.&lt;/b&gt;&lt;br /&gt;Set this higher if your voice breaks up when you speak (seen by a rapidly blinking voice icon next to your name).</source>
        <translation>&lt;b&gt;Détermine combien de temps la transmission devrait continuer après une pause.&lt;/b&gt;&lt;br /&gt;Choisissez une valeur supérieure si votre voix est coupée lorsque vous parlez (l&apos;icône située a coté de votre nom clignote alors très rapidement).&lt;br /&gt;Cela n&apos;a de sens que si vous utilisez la transmission selon l&apos;Activité Vocale.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Silence Below</source>
        <translation>Silence en deça</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values below this count as silence</source>
        <translation>Les valeurs de signal en deça sont du silence</translation>
    </message>
    <message>
        <location line="+3"/>
        <location line="+32"/>
        <source>&lt;b&gt;This sets the trigger values for voice detection.&lt;/b&gt;&lt;br /&gt;Use this together with the Audio Statistics window to manually tune the trigger values for detecting speech. Input values below &quot;Silence Below&quot; always count as silence. Values above &quot;Speech Above&quot; always count as voice. Values in between will count as voice if you&apos;re already talking, but will not trigger a new detection.</source>
        <translation>&lt;b&gt;Définit la valeur pour la détection de la voix.&lt;/b&gt;&lt;br/&gt;Utilisez ce réglage avec la fenêtre de Statistiques Audio pour régler manuellement la valeur de détection de la voix. Toute valeur d&apos;entrée en dessous de &quot;Silence en deça&quot; sera toujours considérée comme du silence. Les valeurs au delà de &quot;Voix au delà&quot; seront toujours considérées comme de la voix. Les valeurs intermédiaires seront considérées comme étant de la voix si vous êtes en train de parler, mais n&apos;activeront pas une nouvelle détection.</translation>
    </message>
    <message>
        <location line="-10"/>
        <source>Speech Above</source>
        <translation>Voix au delà</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal values above this count as voice</source>
        <translation>Les valeurs de signal au delà comptent comme de la voix</translation>
    </message>
    <message>
        <location line="+38"/>
        <source>empty</source>
        <translation>vide</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Audio Processing</source>
        <translation>Processeur audio</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Noise Suppression</source>
        <translation>Suppression du bruit</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Noise suppression</source>
        <translation>Suppression du bruit</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This sets the amount of noise suppression to apply.&lt;/b&gt;&lt;br /&gt;The higher this value, the more aggressively stationary noise will be suppressed.</source>
        <translation>&lt;b&gt;Définit la quantité de son à supprimer à appliquer.&lt;/b&gt;&lt;br/&gt;Plus la valeur est élevée, plus les bruits seront supprimés.</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Amplification</source>
        <translation>Amplification</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Maximum amplification of input sound</source>
        <translation>Amplification maximale de l&apos;entrée sonore</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;Maximum amplification of input.&lt;/b&gt;&lt;br /&gt;RetroShare normalizes the input volume before compressing, and this sets how much it&apos;s allowed to amplify.&lt;br /&gt;The actual level is continually updated based on your current speech pattern, but it will never go above the level specified here.&lt;br /&gt;If the &lt;i&gt;Microphone loudness&lt;/i&gt; level of the audio statistics hover around 100%, you probably want to set this to 2.0 or so, but if, like most people, you are unable to reach 100%, set this to something much higher.&lt;br /&gt;Ideally, set it so &lt;i&gt;Microphone Loudness * Amplification Factor &gt;= 100&lt;/i&gt;, even when you&apos;re speaking really soft.&lt;br /&gt;&lt;br /&gt;Note that there is no harm in setting this to maximum, but RetroShare will start picking up other conversations if you leave it to auto-tune to that level.</source>
        <translation>&lt;b&gt;Amplification de l&apos;entrée maximale.&lt;/b&gt;&lt;br /&gt;Retroshare normalise le son de l&apos;entrée avant de le compresser, et l&apos;amplifie aussi fort qu&apos;il en est autorisé.&lt;br /&gt;Le niveau actuel est continuellement mis à jour selon votre modèle de parole actuel, mais il ne dépassera jamais le niveau spécifié ici.&lt;br /&gt;Si le &lt;i&gt;Volume du microphone&lt;/li&gt; des statistiques audio tourne autour de 100%, vous voudrez probablement le mettre à 2.0 ou quelque chose comme ça, mais si, comme la plupart des gens, vous n&apos;arrivez pas à atteindre 100%, définissez quelque chose de beaucoup plus haut.&lt;br /&gt;Idéallement, définissez le de manière à ce que &lt;i&gt;Volume du microphone * Facteur d&apos;amplification &gt;= 100&lt;/i&gt;, même si vous parlez vraiment doucement.&lt;br /&gt;&lt;br /&gt;Notez que mettre cette option au maximum ne causera aucun dégât, mais que Retroshare risque d&apos;entendre d&apos;autres conversations, si vous le laissez à ce niveau.</translation>
    </message>
    <message>
        <location line="+32"/>
        <source>Echo Cancellation Processing</source>
        <translation>Annulation d&apos;écho en cours</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video Processing</source>
        <translation>Traitement vidéo</translation>
    </message>
    <message>
        <location line="+43"/>
        <source>Available bandwidth:</source>
        <translation>Bande passante disponible :</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Use this field to simulate the maximum bandwidth available so as to preview what the encoded video will look like with the corresponding compression rate.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Utilisez ce champ pour simuler la largeur de bande passante maximale disponible afin de prévisualiser ce à quoi la vidéo codée ressemblera avec le taux de compression correspondant.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>KB/s</source>
        <translation>KO/s</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Display encoded (and then decoded) frame, to check the codec&apos;s quality. If not selected, the image above only shows the frame that is grabbed from your camera.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</source>
        <translation>&lt;html&gt;&lt;head/&gt;&lt;body&gt;&lt;p&gt;Trame d&apos;affichage encodée (et décodée), afin de vérifier la qualité du codec. Si non sélectionnée, l&apos;image ci-dessus montre seulement le cadre qui est saisi depuis votre caméra.&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>preview</source>
        <translation>prévisualisation</translation>
    </message>
</context>
<context>
    <name>AudioInputConfig</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="+202"/>
        <source>Continuous</source>
        <translation>En continu</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Voice Activity</source>
        <translation>Activité vocale</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Push To Talk</source>
        <translation>Pressez pour parler</translation>
    </message>
    <message>
        <location line="+105"/>
        <source>%1 s</source>
        <translation>%1 s</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Off</source>
        <translation>Off</translation>
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
        <translation>Statistiques audio</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Input Levels</source>
        <translation>Niveaux d&apos;entrée</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Peak microphone level</source>
        <translation>Niveau maximal du microphone</translation>
    </message>
    <message>
        <location line="+7"/>
        <location line="+20"/>
        <location line="+20"/>
        <source>Peak power in last frame</source>
        <translation>Puissance maximale dans la trame précédente</translation>
    </message>
    <message>
        <location line="-37"/>
        <source>This shows the peak power in the last frame (20 ms), and is the same measurement as you would usually find displayed as &quot;input power&quot;. Please disregard this and look at &lt;b&gt;Microphone power&lt;/b&gt; instead, which is much more steady and disregards outliers.</source>
        <translation>Montre la puissance maximale de la dernière trame (20 ms), c&apos;est la même valeur que vous trouverez normalement en &quot;puissance d&apos;entrée&quot;.Veuillez ignorer cette valeur et regarder plutôt la &lt;b&gt;puissance du micro&lt;/b&gt;, qui est beaucoup plus stable et ne tient pas compte des valeurs aberrantes.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak speaker level</source>
        <translation>Niveau maximal du haut-parleur</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power of the speakers in the last frame (20 ms). Unless you are using a multi-channel sampling method (such as ASIO) with speaker channels configured, this will be 0. If you have such a setup configured, and this still shows 0 while you&apos;re playing audio from other programs, your setup is not working.</source>
        <translation>Montre la puissance maximale de la dernière trame (20 ms) des haut-parleurs. Sauf si vous utilisez une méthode multicanal d&apos;échantillonnage (telles que l&apos;ASIO) des canaux des haut-parleurs configuré, cette valeur sera 0. Si vous avez une telle configuration, affichant toujours 0 pendant que vous entendez de le son d&apos;autres applications, votre configuration ne fonctionne pas.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Peak clean level</source>
        <translation>Niveau maximal nettoyé</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>This shows the peak power in the last frame (20 ms) after all processing. Ideally, this should be -96 dB when you&apos;re not talking. In reality, a sound studio should see -60 dB, and you should hopefully see somewhere around -20 dB. When you are talking, this should rise to somewhere between -5 and -10 dB.&lt;br /&gt;If you are using echo cancellation, and this rises to more than -15 dB when you&apos;re not talking, your setup is not working, and you&apos;ll annoy other users with echoes.</source>
        <translation>Montre la puissance maximale de la dernière trame (20 ms) après tous les traitements. Idéalement, elle doit être de -96dB quand vous ne parlez pas. En réalité, un studio sonore atteindra -60 dB, et vous aurrez joyeusement -20 dB ailleurs. Quand vous ne parlez pas, il devrait se trouver entre -5 et -10 dB.&lt;br/&gt;Si vous utiliser la suppression de l&apos;écho, et que la valeur est de plus de -15 dB quand vous ne parlez pas, votre configuration ne fonctionne pas, et vous dérangez les autres utilisateurs avec l&apos;écho.</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Signal Analysis</source>
        <translation>Analyse du signal</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Microphone power</source>
        <translation>Puissance du micro</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>How close the current input level is to ideal</source>
        <translation>À quel point le niveau sonore en entrée est proche de l&apos;idéal</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows how close your current input volume is to the ideal. To adjust your microphone level, open whatever program you use to adjust the recording volume, and look at the value here while talking.&lt;br /&gt;&lt;b&gt;Talk loud, as you would when you&apos;re upset over getting fragged by a noob.&lt;/b&gt;&lt;br /&gt;Adjust the volume until this value is close to 100%, but make sure it doesn&apos;t go above. If it does go above, you are likely to get clipping in parts of your speech, which will degrade sound quality.</source>
        <translation>Montre à quel point le niveau de volume d&apos;entré courant est proche de l&apos;idéal. Pour ajuster le niveau de votre microphone, ouvrez le programme que vous utilisez pour ajuster le volume d&apos;enregistrement, et regardez la valeur ici pendant que vous parlez.&lt;br /&gt;&lt;b&gt;Parlez fort, comme lorsque vous êtes énervé d&apos;avoir été tué par un noob.&lt;/b&gt;&lt;br /&gt;Réglez le volume jusqu&apos;à ce que cette valeur soit proche de 100%, mais assurez-vous qu&apos;elle ne le dépasse pas. Si ça va au-delà, vous aurez probablement des coupure dans votre discussion, ce qui degradera la qualité audio.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Signal-To-Noise ratio</source>
        <translation>Taux de signal/bruit</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Signal-To-Noise ratio from the microphone</source>
        <translation>Rapport Signal/Bruit du microphone</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the Signal-To-Noise Ratio (SNR) of the microphone in the last frame (20 ms). It shows how much clearer the voice is compared to the noise.&lt;br /&gt;If this value is below 1.0, there&apos;s more noise than voice in the signal, and so quality is reduced.&lt;br /&gt;There is no upper limit to this value, but don&apos;t expect to see much above 40-50 without a sound studio.</source>
        <translation>C&apos;est le rapport Signal/Bruit (SNR) de votre microphone de la dernière trame (20 ms). Il vous affiche comment votre voix est nettoyée des bruits.&lt;br/&gt;Si cette valeur est suppérieure à 1.0, Il y a davantage de bruits que de signal vocal, et la qualité est donc réduite.&lt;br/&gt;Il n&apos;y a pas de limite supérieure à cette valeur, mais n&apos;espérez pas avoir de valeurs supérieures à 40-50 sans studio audio.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Probability</source>
        <translation>Probabilité de parole</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Probability of speech</source>
        <translation>Probabilité de parole</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the probability that the last frame (20 ms) was speech and not environment noise.&lt;br /&gt;Voice activity transmission depends on this being right. The trick with this is that the middle of a sentence is always detected as speech; the problem is the pauses between words and the start of speech. It&apos;s hard to distinguish a sigh from a word starting with &apos;h&apos;.&lt;br /&gt;If this is in bold font, it means RetroShare is currently transmitting (if you&apos;re connected).</source>
        <translation>Il s&apos;agit de la probabilité que la trame précédente (20 ms) soit de la parole et non du bruit.&lt;br /&gt;La transmission de l&apos;activité vocale dépend de cette valeur. L&apos;astuce est que le milieu de la phrase est toujours détecté comme de la voix ; les problèmes sont les pauses entre les mots et le début des phrases. Il est difficile de distinguer un soupir d&apos;un mot commençant par &quot;h&quot;.&lt;br /&gt;Si c&apos;est écrit en gras, cela signifie que Retroshare est en train de transmettre des données (si vous êtes connectés).</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Configuration feedback</source>
        <translation>Retour de configuration</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Current audio bitrate</source>
        <translation>Débit binaire audio actuel</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Bitrate of last frame</source>
        <translation>Débit binaire de la dernière trame</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This is the audio bitrate of the last compressed frame (20 ms), and as such will jump up and down as the VBR adjusts the quality. The peak bitrate can be adjusted in the Settings dialog.</source>
        <translation>Il s&apos;agit du débit binaire de la dernière trame compressée (20 ms). Ceci va varier de haut en bas puisque nous utilisons un bitrate variable pour ajuster la qualité. Pour ajuster le bitrate maximal, ajustez &lt;b&gt;Complexité de la compression&lt;/b&gt; dans les Préférences.</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>DoublePush interval</source>
        <translation>Intervalle double PTT</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Time between last two Push-To-Talk presses</source>
        <translation>Temps entre la dernière pression double sur Appuyez-pour-Parler</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Speech Detection</source>
        <translation>Détection de parole</translation>
    </message>
    <message>
        <location line="+7"/>
        <source>Current speech detection chance</source>
        <translation>Détection de parole actuelle</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>&lt;b&gt;This shows the current speech detection settings.&lt;/b&gt;&lt;br /&gt;You can change the settings from the Settings dialog or from the Audio Wizard.</source>
        <translation>&lt;b&gt;Ceci affiche le niveau actuel des paramètres de détection de parole.&lt;/b&gt;&lt;br /&gt;Vous pouvez changer ses paramètres depuis les paramètres ou l&apos;assistant audio.</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Signal and noise power spectrum</source>
        <translation>Spectre de la puissance du signal d&apos;entrée et du bruit estimé</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Power spectrum of input signal and noise estimate</source>
        <translation>Spectre de la puissance du signal d&apos;entrée et du bruit estimé</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the power spectrum of the current input signal (red line) and the current noise estimate (filled blue).&lt;br /&gt;All amplitudes are multiplied by 30 to show the interesting parts (how much more signal than noise is present in each waveband).&lt;br /&gt;This is probably only of interest if you&apos;re trying to fine-tune noise conditions on your microphone. Under good conditions, there should be just a tiny flutter of blue at the bottom. If the blue is more than halfway up on the graph, you have a seriously noisy environment.</source>
        <translation>Montre le spectre de puissance du signal d&apos;entrée actuel (ligne rouge) et l&apos;estimation du bruit actuel (en bleu).&lt;br /&gt;Toutes les amplitudes sont multipliées par 30 pour montrer les parties intéressantes (quantité de signal par rapport au bruit dans chaque bande de fréquence).&lt;br /&gt;Ce n&apos;est intéressant que si vous voulez configurer finement les conditions de bruit de votre microphone. Dans de bonnes conditions, il devrait y avoir seulement un léger battement de bleu. Si le niveau de bleu dépasse la moitié sur la graphique, vous avez un envireonnement vraiment bruyant.</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Echo Analysis</source>
        <translation>Analyse de l&apos;écho</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Weights of the echo canceller</source>
        <translation>Force de l&apos;écho suppression</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>This shows the weights of the echo canceller, with time increasing downwards and frequency increasing to the right.&lt;br /&gt;Ideally, this should be black, indicating no echo exists at all. More commonly, you&apos;ll have one or more horizontal stripes of bluish color representing time delayed echo. You should be able to see the weights updated in real time.&lt;br /&gt;Please note that as long as you have nothing to echo off, you won&apos;t see much useful data here. Play some music and things should stabilize. &lt;br /&gt;You can choose to view the real or imaginary parts of the frequency-domain weights, or alternately the computed modulus and phase. The most useful of these will likely be modulus, which is the amplitude of the echo, and shows you how much of the outgoing signal is being removed at that time step. The other viewing modes are mostly useful to people who want to tune the echo cancellation algorithms.&lt;br /&gt;Please note: If the entire image fluctuates massively while in modulus mode, the echo canceller fails to find any correlation whatsoever between the two input sources (speakers and microphone). Either you have a very long delay on the echo, or one of the input sources is configured wrong.</source>
        <translation>Montre le poids de la suppression de l&apos;écho, avec le temps en ordonnée et la fréquence en absisse.&lt;br /&gt;Idéalement, celui-ci devrait être noir, indiquant qu&apos;aucun écho n&apos;existe. Plus communément, vous aurez une ou plusieurs bandes bleutées représentant le temps de retard de l&apos;écho. Vous devriez être capable de visualiser les poids mis à jour en temps réel.&lt;br /&gt;Notez que tant que vous n&apos;avez aucun signal où enlever l&apos;écho, vous ne verrez pas beaucoup de données utiles ici. Ecoutez de la musique et les choses devraient se stabiliser.&lt;br /&gt;Vous pouvez choisir de voir la partie réelle ou imaginaire dans le domaine fréquentiel, ou alternativement le module et la phase des poids. Le plus utile de ces outils serait probablement le module, qui est l&apos;amplitude de l&apos;écho, qui montre la quantité de signal sortant supprimé à chaque instant. Les autres modes de visualisation sont principalement utiles pour les gens qui veulent régler les algorithmes d&apos;annulation de l&apos;écho.&lt;br /&gt;Remarque : si l&apos;image entière fluctue massivement en mode module, l&apos;algorithme de suppression de l&apos;écho n&apos;a pas réussi à trouver une corrélation entre les deux sources d&apos;entrée (haut-parleurs et microphone). Soit vous avez un retard très long sur l&apos;écho, soit une des sources d&apos;entrée est mal configurée.</translation>
    </message>
</context>
<context>
    <name>AudioWizard</name>
    <message>
        <location filename="../gui/AudioWizard.ui" line="+14"/>
        <source>Audio Tuning Wizard</source>
        <translation>Assistant audio</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Introduction</source>
        <translation>Introduction</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Welcome to the RetroShare Audio Wizard</source>
        <translation>Bienvenue dans l&apos;assistant audio de RetroShare</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This is the audio tuning wizard for RetroShare. This will help you correctly set the input levels of your sound card, and also set the correct parameters for sound processing in Retroshare. </source>
        <translation>Ceci est l&apos;assistant audio pour RetroShare. Il vous aide à configurer correctement le le niveau d&apos;entré de votre carte son, Et également définir les paramètres corrects dans le traitement du son pour RetroShare.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Volume tuning</source>
        <translation>Réglage du volume</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Tuning microphone hardware volume to optimal settings.</source>
        <translation>Réglage du volume matériel du micro pour une meilleure configuration.</translation>
    </message>
    <message>
        <source>&lt;p &gt;Open your sound control panel and go to the recording settings. Make sure the microphone is selected as active input with maximum recording volume. If there&apos;s an option to enable a &amp;quot;Microphone boost&amp;quot; make sure it&apos;s checked. &lt;/p&gt;
&lt;p&gt;Speak loudly, as when you are annoyed or excited. Decrease the volume in the sound control panel until the bar below stays as high as possible in the green and orange but not the red zone while you speak. &lt;/p&gt;</source>
        <translation type="vanished">&lt;p&gt;Ouvrir le panneau de contrôle de votre carte son et aller dans les options d&apos;enregistrements. Vérifiez que le microphone est séléctionné en tant qu&apos;entrée active avec le volume d&apos;enregistrement au maximum. S&apos;il ya une option pour permettre le  &amp;quot;mic boost&amp;quot; vérifiez qu&apos;il est cochée. &lt;/p&gt;
&lt;p&gt;Parler fort, comme lorsque vous êtes ennuyé ou excitée. Diminuer le volume sonore dans le panneau de contrôle jusqu&apos;à ce que la barre soit aussi haute que possible dans le bleu et le vert, mais &lt;b&gt;pas&lt;/b&gt; dans la zone rouge pendant que vous parlez. &lt;/p&gt;</translation>
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
        <translation>Parlez normalement, et ajustez le curseur ci-dessous pour que la barre se déplace dans le vert lorsque vous parlez, et n&apos;entre dans la zone orange.</translation>
    </message>
    <message>
        <location line="+44"/>
        <source>Stop looping echo for this wizard</source>
        <translation>Arrêter la boucle d&apos;écho pour cet assistant</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Apply some high contrast optimizations for visually impaired users</source>
        <translation>Applique quelques optimisations de contraste pour les utilisateurs malvoyants</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Use high contrast graphics</source>
        <translation>Augmenter le contraste</translation>
    </message>
    <message>
        <location line="+10"/>
        <source>Voice Activity Detection</source>
        <translation>Détection de l&apos;Activité Vocale</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Letting RetroShare figure out when you&apos;re talking and when you&apos;re silent.</source>
        <translation>RetroShare détermine quand vous parlez, et lorsque vous faites le silence.</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>This will help Retroshare figure out when you are talking. The first step is selecting which data value to use.</source>
        <translation>Cela permettra de RetroShare savoir quand vous parlez. La première étape consiste à sélectionner les données de valeur à utiliser.</translation>
    </message>
    <message>
        <location line="+12"/>
        <source>Push To Talk:</source>
        <translation>Appuyez pour parler :</translation>
    </message>
    <message>
        <location line="+29"/>
        <source>Voice Detection</source>
        <translation>Détection de voix</translation>
    </message>
    <message>
        <location line="+13"/>
        <source>Next you need to adjust the following slider. The first few utterances you say should end up in the green area (definitive speech). While talking, you should stay inside the yellow (might be speech) and when you&apos;re not talking, everything should be in the red (definitively not speech).</source>
        <translation>Ensuite, vous devez ajuster les deux curseurs. Les premières syllabes des phares devraient se trouver dans la zone verte (voix confirmée). Tout en parlant, vous devriez rester à l&apos;intérieur du jaune (voix hypotétique), et lorsque vous ne parlez pas, tout devrait être dans le rouge (aucune voix).</translation>
    </message>
    <message>
        <location line="+64"/>
        <source>Continuous transmission</source>
        <translation>Transmission continue</translation>
    </message>
    <message>
        <location line="+8"/>
        <source>Finished</source>
        <translation>Terminé</translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Enjoy using RetroShare</source>
        <translation>Profitez de l&apos;utilisation de RetroShare</translation>
    </message>
    <message>
        <location line="+6"/>
        <source>Congratulations. You should now be ready to enjoy a richer sound experience with Retroshare.</source>
        <translation>Félicitations. Vous devriez maintenant être prêt à profiter d&apos;une expérience sonore riche avec RetroShare.</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../VOIPPlugin.cpp" line="+128"/>
        <source>&lt;h3&gt;RetroShare VOIP plugin&lt;/h3&gt;&lt;br/&gt;   * Contributors: Cyril Soler, Josselin Jacquard&lt;br/&gt;</source>
        <translation>&lt;h3&gt;Extension RetroShare VOIP&lt;/h3&gt;&lt;br/&gt;   * Contributeurs: Cyril Soler, Josselin Jacquard&lt;br/&gt;</translation>
    </message>
    <message>
        <source>&lt;br/&gt;The VOIP plugin adds VOIP to the private chat window of RetroShare. to use it, proceed as follows:&lt;UL&gt;</source>
        <translation type="vanished">&lt;br/&gt;L&apos;extension VOIP ajoute la voip à la fenêtre du chat privé de RetroShare. Pour l&apos;utiliser, faîtes ce qui suit :&lt;UL&gt;</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>&lt;li&gt; setup microphone levels using the configuration panel&lt;/li&gt;</source>
        <translation>&lt;li&gt; Régler le niveau du microphone en utilisant le panneau de configuration&lt;/li&gt;</translation>
    </message>
    <message>
        <source>&lt;li&gt; check your microphone by looking at the VU-metters&lt;/li&gt;</source>
        <translation type="vanished">&lt;li&gt; vérifier votre microphone en regardant les VU-mètres&lt;/li&gt;</translation>
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
        <translation>&lt;li&gt; dans le chat privé, activé l&apos;entrée/sortie du son en cliquant sur les deux icônes de la VOIP&lt;/li&gt;&lt;/ul&gt;</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Your friend needs to run the plugin to talk/listen to you, or course.</source>
        <translation>Votre ami doit  bien sûr pour parler/entendre lui aussi utiliser l&apos;extension.</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>&lt;br/&gt;&lt;br/&gt;This is an experimental feature. Don&apos;t hesitate to send comments and suggestion to the RS dev team.</source>
        <translation>&lt;br/&gt;&lt;br/&gt;Cette fonctionnalité est expérimentale. N&apos;hésitez pas à nous envoyer vos commentaires et suggestions à l&apos;équipe de dev de RS.</translation>
    </message>
</context>
<context>
    <name>VOIP</name>
    <message>
        <location line="+47"/>
        <source>This plugin provides voice communication between friends in RetroShare.</source>
        <translation>Cette extension permet la communication vocale entre amis dans RetroShare.</translation>
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
        <translation>Appel audio entrant</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Incoming video call</source>
        <translation>Appel vidéo entrant</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing audio call</source>
        <translation>Appel audio sortant</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Outgoing video call</source>
        <translation>Appel vidéo sortant</translation>
    </message>
</context>
<context>
    <name>VOIPChatWidgetHolder</name>
    <message>
        <location filename="../gui/VOIPChatWidgetHolder.cpp" line="+70"/>
        <location line="+146"/>
        <source>Mute</source>
        <translation>Muet</translation>
    </message>
    <message>
        <location line="-128"/>
        <location line="+138"/>
        <source>Start Call</source>
        <translation>Lancer l&apos;appel</translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Start Video Call</source>
        <translation>Lancer l&apos;appel vidéo</translation>
    </message>
    <message>
        <location line="-121"/>
        <location line="+131"/>
        <source>Hangup Call</source>
        <translation>Raccrocher l&apos;appel</translation>
    </message>
    <message>
        <location line="-113"/>
        <location line="+626"/>
        <source>Hide Chat Text</source>
        <translation>Cacher le texte de tchat</translation>
    </message>
    <message>
        <location line="-608"/>
        <location line="+106"/>
        <location line="+523"/>
        <source>Fullscreen mode</source>
        <translation>Mode plein écran</translation>
    </message>
    <message>
        <source>%1 inviting you to start an audio conversation. Do you want Accept or Decline the invitation?</source>
        <translation type="vanished">%1 vous invite à commencer une conversation en audio. Voulez-vous accepter ou décliner l&apos;invitation ?</translation>
    </message>
    <message>
        <location line="-410"/>
        <source>Accept Audio Call</source>
        <translation>Accepter l&apos;appel audio</translation>
    </message>
    <message>
        <location line="+17"/>
        <source>Decline Audio Call</source>
        <translation>Décliner l&apos;appel audio</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse audio call</source>
        <translation>Refuser appel audio</translation>
    </message>
    <message>
        <source>%1 inviting you to start a video conversation. Do you want Accept or Decline the invitation?</source>
        <translation type="vanished">%1 vous invite à commencer une conversation en vidéo. Voulez-vous accepter ou décliner l&apos;invitation ?</translation>
    </message>
    <message>
        <location line="+52"/>
        <source>Decline Video Call</source>
        <translation>Décliner l&apos;appel vidéo</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Refuse video call</source>
        <translation>Refuser l&apos;appel vidéo</translation>
    </message>
    <message>
        <location line="+102"/>
        <source>Mute yourself</source>
        <translation>Mode muet</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Unmute yourself</source>
        <translation>Se démuter</translation>
    </message>
    <message>
        <source>Waiting your friend respond your video call.</source>
        <translation type="vanished">En attente que votre ami réponde à votre appel vidéo.</translation>
    </message>
    <message>
        <location line="+603"/>
        <source>Your friend is calling you for video. Respond.</source>
        <translation>Votre ami vous appelle en vidéo. Répondre.</translation>
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
        <translation>Statut VoIP</translation>
    </message>
    <message>
        <location line="-467"/>
        <source>Hold Call</source>
        <translation>Tenir l&apos;appel</translation>
    </message>
    <message>
        <location line="+20"/>
        <source>Outgoing Call is started...</source>
        <translation>Appel sortant lancé...</translation>
    </message>
    <message>
        <location line="+9"/>
        <source>Resume Call</source>
        <translation>Reprendre l&apos;appel</translation>
    </message>
    <message>
        <location line="+16"/>
        <source>Outgoing Audio Call stopped.</source>
        <translation>Appel audio sortant stoppé.</translation>
    </message>
    <message>
        <location line="+46"/>
        <source>Shut camera off</source>
        <translation>Couper la caméra</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>You&apos;re now sending video...</source>
        <translation>Vous envoyez maintenant de la vidéo ...</translation>
    </message>
    <message>
        <location line="-266"/>
        <location line="+279"/>
        <source>Activate camera</source>
        <translation>Activer caméra</translation>
    </message>
    <message>
        <location line="+15"/>
        <source>Video call stopped</source>
        <translation>Appel vidéo stoppé</translation>
    </message>
    <message>
        <location line="-295"/>
        <source>Accept Video Call</source>
        <translation>Accepter l&apos;appel vidéo</translation>
    </message>
    <message>
        <location line="-55"/>
        <source>%1 is inviting you to start an audio conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+3"/>
        <source>Activate audio</source>
        <translation>Activer l&apos;audio</translation>
    </message>
    <message>
        <location line="+50"/>
        <source>%1 is inviting you to start a video conversation. Do you want to Accept or Decline the invitation?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location line="+334"/>
        <source>Show Chat Text</source>
        <translation>Montrer le texte de tchat</translation>
    </message>
    <message>
        <location line="+19"/>
        <source>Return to normal view.</source>
        <translation>Retourner en vue normale.</translation>
    </message>
    <message>
        <location line="+228"/>
        <source>%1 hang up. Your call is closed.</source>
        <translation>%1 raccroche. Votre appel est fermé.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your audio call is closed.</source>
        <translation>%1 raccroche. Votre appel audio est fermé.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 hang up. Your video call is closed.</source>
        <translation>%1 raccroche. Votre appel vidéo est fermé.</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>%1 accepted your audio call.</source>
        <translation>%1 a accepté votre appel audio.</translation>
    </message>
    <message>
        <location line="+11"/>
        <source>%1 accepted your video call.</source>
        <translation>%1 a accepté votre appel vidéo.</translation>
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
        <translation type="vanished">En attente que votre ami réponde à votre appel audio.</translation>
    </message>
    <message>
        <location line="-44"/>
        <source>Your friend is calling you for audio. Respond.</source>
        <translation>Votre ami vous appelle en audio. Répondre.</translation>
    </message>
    <message>
        <location line="+17"/>
        <location line="+58"/>
        <source>Answer</source>
        <translation>Répondre</translation>
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
        <translation>Répondre</translation>
    </message>
    <message>
        <location line="+4"/>
        <source>Answer with video</source>
        <translation>Répondre avec vidéo</translation>
    </message>
    <message>
        <location filename="../gui/VOIPToasterItem.ui" line="+232"/>
        <source>Decline</source>
        <translation>Décliner</translation>
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
        <translation>Accepter</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Bandwidth Information</source>
        <translation>Information bande passante</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Audio or Video Data</source>
        <translation>Données audio ou vidéo</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>HangUp</source>
        <translation>Raccrocher</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Invitation</source>
        <translation>Invitation</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Audio Call</source>
        <translation>Appel audio</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Video Call</source>
        <translation>Appel vidéo</translation>
    </message>
    <message>
        <location line="+110"/>
        <source>Test VOIP Accept</source>
        <translation>Tester l&apos;acceptation de VOIP</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP BandwidthInfo</source>
        <translation>Tester BandwidthInfo de VOIP</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Data</source>
        <translation>Tester données VOIP</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP HangUp</source>
        <translation>Test raccrochage VOIP</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Invitation</source>
        <translation>Test invitation VOIP</translation>
    </message>
    <message>
        <location line="+2"/>
        <source>Test VOIP Audio Call</source>
        <translation>Test appel audio VOIP</translation>
    </message>
    <message>
        <location line="+1"/>
        <source>Test VOIP Video Call</source>
        <translation>Test appel vidéo VOIP</translation>
    </message>
    <message>
        <location line="+21"/>
        <source>Accept received from this peer.</source>
        <translation>Acceptation reçue depuis ce pair.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Bandwidth Info received from this peer: %1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Bandwidth Info received from this peer:%1</source>
        <translation type="vanished">Info de bande passante reçue depuis ce pair :%1</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Audio or Video Data received from this peer.</source>
        <translation>Données audio ou vidéo reçue depuis ce pair.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>HangUp received from this peer.</source>
        <translation>Raccrochage reçu depuis ce pair.</translation>
    </message>
    <message>
        <location line="+24"/>
        <source>Invitation received from this peer.</source>
        <translation>Invitation reçue depuis ce pair.</translation>
    </message>
    <message>
        <location line="+25"/>
        <location line="+24"/>
        <source>calling</source>
        <translation>appel en cours</translation>
    </message>
</context>
<context>
    <name>voipGraphSource</name>
    <message>
        <location filename="../gui/AudioInputConfig.cpp" line="-260"/>
        <source>Required bandwidth</source>
        <translation>Bande passante requise</translation>
    </message>
</context>
</TS>
