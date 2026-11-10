# RaBASS 0.3.5 — UI Preview 06: Script Logo

**Subtractive synthesis bass synth** za Windows 10/11 x64, u **VST3** formatu.
CMake projekt za Visual Studio 2026 i MIDI trake u Ableton Liveu 12.
Isporuka je **prijenosni VST3 ZIP i izvorni projekt**, bez Windows instalera.

Probni UI ima tamnocrvenu brušenu metalnu podlogu, manje kromirane potenciometre
s višim reljefom i izraženijim sjenama, kromirane ručice klizača te uže i
nešto više tipke. Dimenzije su 1140 × 760. Područja za povlačenje kontrola
ostaju dovoljno velika za rad mišem.
Paneli, izbornici, polja s vrijednostima i oznake koriste usklađenu
tamnocrvenu paletu; kromirane kontrole i njihov reljef ostaju sačuvani.
Naziv **RaBass** u zaglavlju sada je rukopisni logotip sa spojenim slovima,
u svijetloj boji na tamnocrvenoj podlozi. Logotip je ugrađen u plugin pa
ne zahtijeva instalaciju dodatnog fonta.
Oscilatori, filter, envelope, output i preseti imaju zaseban prostor.
Izbornik preseta ima odjeljke **Factory presets** i **User presets**,
tipku **SAVE PRESET** te tipke za prethodni/sljedeći preset.
DSP engine i izvornih šest tvorničkih zvukova nisu mijenjani.
Prethodna [stabilna verzija 0.2.0](https://github.com/voyagersquare/RaBASS/releases/tag/v0.2.0)
ostaje dostupna dok isprobavamo novi izgled.

![RaBASS instrument](Assets/RaBASS-editor.png)

## Oscilatori i miks

**SAWTOOTH**, **TRIANGLE** i **SINE** imaju nezavisne razine od 0 do 100 %.
Možete koristiti svaki val samostalno ili ih miješati u bilo kojem omjeru.
Miks ulazi u postojeći rezonantni low-pass filter, zatim u amplitude ADSR
te izlazni stupanj. Filter envelope otvara cutoff do šest oktava.

- SAWTOOTH daje bogat spektar za klasični subtractive bass.
- TRIANGLE dodaje mekše, uglavnom neparne harmonike.
- SINE daje temeljni ton za čišći bass.

BLEND i DETUNE zadržavaju svoju ulogu: BLEND miješa osnovnu i detuniranu
kopiju odabranog timbra, a DETUNE određuje razliku njihove visine.
SUB je zaseban sinusni oscilator oktavu niže. Kada su sva tri glavna vala
na 0 %, SUB i dalje može svirati; za potpunu tišinu spustite i SUB na 0 %.
Razine miksera imaju kratko zaglađivanje, a zbroj iznad 100 % normalizira
se radi prostora u izlaznom signalu.

## Precizniji Filter Cutoff

Cutoff se povlačenjem miša podešava četiri puta postupnije, a kotačićem
miša deset puta finije. **Shift + kotačić** daje dodatnih deset puta manji
pomak. Tipke sa strelicama mijenjaju cutoff za **1 Hz**, a uz Shift za
**0,1 Hz**. Polje uvijek prikazuje Hz s jednom decimalom; za izravan unos
možete upisati Hz ili kHz. Raspon 30–18000 Hz, mapiranje automatizacije,
kratko DSP zaglađivanje i spremljeni preseti ostaju sačuvani.

## Ostale funkcije

- Postojeći filter, resonance, filter envelope i njegov decay.
- Amplitude ADSR, drive i output; 2x interno uzorkovanje oscilatora i filtera.
- Mono s prioritetom zadnje note, legatom i glideom ili osam glasova.
- MIDI velocity, pitch bend ±2 polutona, sustain CC64 i PANIC.
- 18 tvorničkih bass preseta: izvornih šest plus 12 novih kombinacija.
- Spremanje i učitavanje vlastitih preseta na disk, dostupnih u svim instancama.
- Automatizacija razina sva tri vala i ostalih aktivnih kontrola.
- Spremanje parametara u DAW projekt, mono/stereo izlaz i miš-klavijatura.

U mono načinu povezane note ne pokreću envelope ponovno. Otpuštanje zadnje
note vraća prethodnu još pritisnutu notu. Glide 0 isključuje klizanje.
Lijevi i desni kanal identični su radi centriranog basa.

## Preuzimanje i Ableton Live 12

Preuzmite `RaBASS-0.3.5-Windows-x64-VST3.zip` iz
[Script Logo Preview izdanja](https://github.com/voyagersquare/RaBASS/releases/tag/v0.3.5-ui-preview.6).
Paket je dostupan i u **Actions → Windows VST3 → uspješan run → Artifacts**.
Releases uključuje izvor RaBASS-a, odgovarajući JUCE source i `SHA256SUMS.txt`.

1. Zatvorite Live i raspakirajte ZIP.
2. Kopirajte **cijelu mapu** `RaBASS.vst3` u
   `C:\Program Files\Common Files\VST3\RaBASS.vst3`.
3. U Liveu otvorite Settings → Plug-Ins i uključite
   **Use VST3 Plug-In System Folders**.
4. Kliknite **Rescan**, pronađite RaBASS i dodajte ga na **MIDI traku**.

Nemojte kopirati samo unutarnji modul iz mape `Contents`.
Pri nadogradnji zamijenite cijelu staru mapu `RaBASS.vst3`.
Na odredišnom računalu nisu potrebni Visual Studio ni dodatni runtime download.

## User presets i novi bass zvukovi

1. Podesite zvuk i kliknite **SAVE PRESET** uz padajući izbornik.
2. Unesite naziv i kliknite **Save**. Spremljeni zvuk odabire se u izborniku.
3. U padajućem izborniku otvorite **User presets** i odaberite svoj zvuk.

Nazivi mogu imati do 64 znaka. Ako naziv već postoji, plugin traži potvrdu
prije zamjene postojećeg preseta. Spremanje ne prekida notu koja svira.
Preset obuhvaća sve postavke zvuka, uključujući tri vala, filter, ADSR,
mono/poly, glide, sub, drive i output.

Na Windowsu se `.rabasspreset` datoteke čuvaju u:
`%APPDATA%\Independent Audio\RaBASS\Presets`.
Za sigurnosnu kopiju prenesite cijelu tu mapu. Novi preseti dostupni su i
nakon ponovnog pokretanja Livea; druge otvorene instance osvježavaju popis
kad otvorite izbornik. Projekt sprema zvuk i naziv čak i ako preset datoteka
na drugom računalu nije dostupna. Oštećene datoteke ne učitavaju se.

Novi tvornički preseti:

- Round Finger, Saw Punch, Triangle Pluck, Sine Weight.
- Warm Circuit, Acid Snap, Long Slide, Dub Foundation.
- Grit Bass, Hollow Motion, Tight Sequence, Poly Brass.

## Build u Visual Studiju 2026

Potrebni su **Desktop development with C++**, Windows SDK,
**C++ CMake tools for Windows** i Git for Windows.

1. Klonirajte repozitorij, primjerice u `C:\Dev\RaBASS`.
2. Pokrenite `Build-Windows.cmd`: automatski pronalazi najnoviji VS s x64
   C++ alatima preko vswhere, koristi Ninja i pokreće testove.
3. Za prijenosni paket pokrenite:

```powershell
powershell -NoProfile -File Package-VST3.ps1 -SkipBuild
```

Alternativno otvorite mapu u VS 2026, odaberite CMake preset `windows-x64`
i cilj `RaBASS_VST3`. Koristite x64 toolchain.
JUCE 8.0.6 fiksiran je na commit `51a8a6d7aeae7326956d747737ccf1575e61e209`.
MSVC runtime statički je uključen u plugin.

VST3 izlaz: `build/windows-x64/RaBASS_artefacts/Release/VST3/RaBASS.vst3`.
ZIP i izvorne arhive: `build/packages`.

## Stariji projekti i provjere

Stanja iz 0.1.0 učitavaju se uz zadržavanje filtera, envelopes, suba, detunea
te ostalih parametara. Stari Saw prelazi u Sawtooth, Triangle u Triangle,
a Pulse u približan Sawtooth/Triangle miks. Zvuk stare Pulse postavke zato
neće biti identičan. Nova automatizacija valova koristi tri zasebne razine;
stara automatizacija izbora Waveform više ne upravlja timbrom.

Testovi pokrivaju svaki val samostalno, njihov miks, mutiranje, zasebni sub,
tuning, MIDI, mono prioritet, sustain, polifoniju, razne sample rateove,
spremanje novih razina, obnovu starih stanja i ponovno otvaranje editora.
Provjeravaju i da su sve kontrole unutar prozora te da se međusobno ne
preklapaju i da ih oznake ne zaklanjaju.
Testovi User Presets provjeravaju spremanje, ponovno učitavanje, zamjenu,
rad u novoj instanci, DAW recall, neispravne datoteke i odjeljke izbornika.
Windows workflow stvara i provjerava VST3 ZIP i pripadajuće izvore.
Ne preuzima Inno Setup i ne izrađuje instalacijski EXE.

DSP testovi bez JUCE-a:

```sh
cmake -S . -B build/dsp -DRABASS_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/dsp
ctest --test-dir build/dsp --output-on-failure
```

Stvarni Ableton Live 12 i lokalni Visual Studio 2026 trebaju i provjeru na
korisnikovom Windows računalu: scan, playback, automation i save/reopen.

## Licence

Izvorni RaBASS kod: **CC0-1.0**; vidi [LICENSE](LICENSE).
Kombinirani JUCE plugin koristi JUCE-ovu **AGPLv3** opciju i pripadajuće
licence ovisnosti. CC0 ne mijenja JUCE-ove uvjete. Izvori RaBASS-a i JUCE-a
isporučuju se uz VST3 ZIP. Zatvorena distribucija kombiniranog plugina
zahtijeva odgovarajuću komercijalnu JUCE licencu.
