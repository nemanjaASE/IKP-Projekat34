# Servis za distribuirano skladištenje podataka

## Zadatak 

Razviti servis za distribuirano skladištenje podataka u cilju postizanja visoke pouzdanosti.
Distribuirana baza se sastoji od proizvoljnog broja čvorova koji međusobno komuniciraju, a takođe
svaki čvor može biti i pristupni servis. Upis podataka u bazu treba da bude distribuirana transakcija
koja se izvršava kroz 2PC protokol. Takođe, prilikom pokretanja novog čvora, čvor trebe da se
poveže sa svim ostalim čvorovima u mreži i da uradi integrity update.
Klijent treba da ima mogućnost pristupa bilo kom čvoru mreže i slanje podataka o studentu. Nakon
slanja zahteva, klijent ostaje aktivan i nudi mogućnost ponovnog slanja sve dok se ne unese
komanda za kraj.

## Ciljevi

- Razviti odgovarajući protokol za komunikaciju između komponenti sistema
- Opravdana upotreba niti
- Opravdana upotreba struktura podataka za odgovarajući problem
- Implementacija Proizvođač-Potrošač šablona

## Testiranje

- Izmeriti propusnost sistema sa 1, 3 i 5 čvorova
- Provera curenja memorije
- Adekvatno rukovanje sa završetkom instance programa
- Zatvaranje konekcija
