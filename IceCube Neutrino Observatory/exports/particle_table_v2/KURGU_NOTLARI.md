# Temel parçacıklar ve kozmik nötrinolar — v2

38 saniye, 3840 × 2160, 30 fps, sessiz MP4. Ayrı 1080p önizleme, Türkçe SRT ve timing.json bulunur. Ses kaydı olmadığından anlatım süreleri tahminidir. Ekranda yalnızca evrensel parçacık sembolleri vardır; Türkçe açıklama, başlık veya özellik listesi yoktur.

## Akış

- 0–3 s: ışık dalgası ve γ simgesi.
- 3–8 s: γ, büyük foton kutusuna yerleşir.
- 8–15 s: foton kutusu tablodaki yerine gider. Kuarklar, leptonlar, kuvvet taşıyıcıları ve Higgs ayrı, uyumlu renklerle açılır.
- 15–21 s: tablo okunur halde kalır; γ ile νe, νμ ve ντ kutuları öne çıkar.
- 21–26 s: üç nötrino kutusu kendi yerlerinden merkeze taşınır. Foton nötrinoya dönüşmez; farklı kutular seçilir.
- 26–38 s: temsili kozmik jet kaynağından tek νμ gelir, buzda etkileşir ve müon üretir. Müon izi çevresindeki DOM’lar sırayla aydınlanır; uzaklaştıkça algılama seyrekleşir. Parçacık etiketleri yol sonunda söner, başlangıca dönmez.

## Tablo ve bilimsel sınırlar

17 kutuluk alışılmış Standart Model düzeni kullanılır: altı kuark, altı lepton, g / γ / Z⁰ / W± ve H. Kutular bütün kuantum durumlarının sayımı değildir: gluonların renk durumları g altında, iki W yük durumu W± altında gruplanır; antiparçacıklar ayrı kutulara açılmaz. Kütle, yük ve spin tabloları bu anlatım için eklenmemiştir. Nötrinoların alt indisleri farklı lepton tatlarını gösterir.

Farklı ailelerdeki vurgu animasyonu gerçek dönme, bozunma veya kütle oranlarını temsil etmez. Tablodaki bütün parçacıkların uzayda serbestçe seyahat ederek Dünya'ya ulaştığı gösterilmez. Kozmik finalde yalnızca nötrino haberci kullanılır.

Kaynak belirli bir gökcisminin veya Nobel kapsamında tanımlanmış tek bir kaynağın yeniden üretimi değildir. Kaynak, uzay yolculuğu ve dedektör tek kompozisyonda sembolik olarak gösterilir; uzaklıklar ve süreler ölçekli değildir. νμ ile müonun yönleri neredeyse aynı tutulur. DOM’lar nötrinoyu doğrudan görmez; müonun ürettiği ışığın algılanması temsil edilir. Olay gerçek IceCube verisinden çizilmemiştir.

## Kaynak kontrolü

- CERN, Standart Model parçacıkları ve aileleri: https://home.web.cern.ch/science/physics/standard-model/
- CERN, Higgs bozonu: https://home.cern/science/physics/higgs-boson/
- IceCube, 2026 Nobel duyurusu: https://icecube.wisc.edu/news/awards/2026/10/francis-halzen-icecube-principal-investigator-wins-2026-physics-nobel-prize/
- NSF, 2026 Nobel ve yüksek enerjili astrofiziksel nötrinolar: https://www.nsf.gov/mps/updates/2026-nobel-prize-physics-awarded-nsf-icecube-neutrino

Kaynak kod: animation/icecube_samples/render_particle_table_v2.py
Yeniden üretim: render_particle_table_v2.py --scale 2

## v2 düzeltmesi

Merkeze taşınan üç nötrino kartı diğer kutular ve ayraç çizgilerinden sonra çizilir; geçiş boyunca ön planda kalır.
