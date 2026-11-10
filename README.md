# RaBASS

Analogni bass instrument za **Windows 10/11 x64**, u **VST3** formatu.
Namijenjen MIDI trakama u Ableton Liveu 12. Zaseban projekt od ChopLaba,
s istim tipom CMake/JUCE builda i Windows instalacijskog čarobnjaka.

## Zvuk i kontrole

- Dva oscilatora: saw, pulse ili triangle; blend i detune drugog oscilatora.
- Sinusni sub oscilator oktavu niže.
- Rezonantni low-pass filter, filter envelope s vlastitim decayem.
- Amplitude ADSR, drive i output; 2x interno uzorkovanje oscilatora i filtera.
- Mono s prioritetom zadnje pritisnute note, legatom i glideom, ili 8 glasova.
- MIDI velocity, pitch bend ±2 polutona, sustain CC64, all notes off i all sound off.
- Šest tvorničkih preseta, miš-klavijatura i PANIC.
- Automatizacija svih parametara i spremanje njihovih vrijednosti u DAW projekt.

U mono načinu nove povezane note ne pokreću envelope ponovno. Otpuštanje
zadnje note vraća prethodnu još pritisnutu notu. Glide je vremenska konstanta
približavanja novoj visini; vrijednost 0 isključuje ga. U poly načinu nove note
počinju na vlastitoj visini, uz prednost otpuštenih glasova pri krađi glasa.
BLEND je udio drugog oscilatora. SUB dodaje sinus oktavu ispod prvog.
ENV AMT otvara filter za najviše šest oktava iznad CUTOFF-a. Bas je centriran,
s identičnim lijevim i desnim kanalom. OUTPUT uključuje blago završno zasićenje.

## Preuzimanje i instalacija

Otvorite **Actions → Windows installer → zadnji uspješan run → Artifacts →
RaBASS-Windows-x64**. Raspakirajte arhivu i pokrenite
`RaBASS-0.1.0-Windows-x64-Setup.exe` iz podmape `build/installer`.
Paket sadrži i SHA256, cijeli VST3 bundle, sliku sučelja i izvorne arhive.
Artifacts su dostupni 30 dana; workflow se može ponovno pokrenuti ručno.

Instaler postavlja **cijeli** `RaBASS.vst3` bundle u
`C:\Program Files\Common Files\VST3\RaBASS.vst3`.
Dokumentacija i licence idu u `C:\Program Files\Independent Audio\RaBASS`.
Na odredišnom računalu nisu potrebni Visual Studio, Inno Setup ni internet.
Deinstalacija: Windows Settings → Apps → Installed apps → RaBASS.

U **Ableton Liveu 12**:

1. Settings → Plug-Ins → **Use VST3 Plug-In System Folders: On**.
2. Kliknite **Rescan**. Po potrebi držite Alt pri kliku za puni rescan.
3. U Browser → Plug-Ins pronađite RaBASS i povucite ga na **MIDI traku**.
4. Armirajte traku ili dodajte MIDI clip. Isprobajte note C1–C3 i presete.

Instaler je nepotpisan. Postojeći projekti i korisnički preseti nisu dio
instalacijskih mapa i deinstaler ih ne briše.

## Build u Visual Studiju 2026

Potrebni su **Desktop development with C++**, Windows SDK,
**C++ CMake tools for Windows**, Git for Windows i, za instaler,
**Inno Setup 6.3+**. Skripte ne instaliraju alate na korisnikovo računalo.

1. Klonirajte ovaj repozitorij u kratku putanju, primjerice `C:\Dev\RaBASS`.
2. Pokrenite `Build-Windows.cmd`; automatski pronalazi najnoviji VS s x64
   C++ alatima preko vswhere. Koristi Ninja, bez vezanja na VS 2022 generator.
3. Pokrenite `Build-Installer.cmd` za gotov `.exe` i njegov SHA256.

Alternativno otvorite mapu u VS 2026 (**File → Open → Folder**), odaberite
`windows-x64` CMake preset i cilj `RaBASS_VST3`. Koristite x64 toolchain.
Prvi build preuzima JUCE 8.0.6, fiksiran na commit
`51a8a6d7aeae7326956d747737ccf1575e61e209`.
MSVC runtime je statički uključen u plugin.

Izlaz: `build/windows-x64/RaBASS_artefacts/Release/VST3/RaBASS.vst3`.
Ne kopirajte samo unutarnji modul; kopirajte cijelu `.vst3` mapu.
Instaler: `build/installer/RaBASS-0.1.0-Windows-x64-Setup.exe`.

## Provjere

Windows workflow kompajlira plugin i testove, provjerava MIDI pomake,
tuning, sustain, mono prioritet, polifoniju, parametre i obnovu stanja,
ponovno otvaranje editora te mono/stereo izlaz. Zatim stvara instaler,
instalira ga na privremeni runner, uspoređuje checksumove svih datoteka,
ponovno instalira radi provjere nadogradnje i deinstalira ga.
Skripta za smoke test namjerno radi samo na GitHub Actions runneru.

Prijenosni DSP test bez JUCE-a:

```sh
cmake -S . -B build/dsp -DRABASS_DSP_ONLY=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/dsp
ctest --test-dir build/dsp --output-on-failure
```

GitHub runner provjerava Windows/MSVC build. Visual Studio 2026 i stvarni
Ableton Live 12 trebaju i ručnu provjeru na korisnikovom Windows računalu:
scan, MIDI playback, automation, save/reopen projekta i nekoliko instanci.
Ovdje se ne tvrdi da je Ableton host test izveden na runneru.

## Licenca

AGPL-3.0-only; vidi [LICENSE](LICENSE). JUCE 8.0.6 koristi se prema njegovoj
AGPLv3 opciji. Windows artifact uključuje izvor ovog projekta i odgovarajući
JUCE source uz njihove licence. Za drukčiji model distribucije potrebno je
uskladiti licencu projekta i odgovarajuću JUCE licencu.
