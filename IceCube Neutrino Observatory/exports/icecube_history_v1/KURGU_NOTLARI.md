# IceCube tarihçesi — v1

70 saniye • 3840 × 2160 • 30 fps • sessiz H.264 MP4.
1080p izleme kopyası, ayrı Türkçe SRT ve timing.json birlikte verilir. Ses kaydı sağlanmadığından senkronizasyon metnin okunma süresine göre tahminidir. Paragraf sınırı 32,5 saniyedir.

## Akış

- 0–4,5: 1987, çizim halindeki dedektör ve fikir simgesi.
- 4,5–9,5: 1990, temsili Grönland silueti ve buz içindeki optik test.
- 9,5–18: 1999, dedektör çiziminin bir teklif dosyası halinde NSF'e iletilmesi. Onay damgası gösterilmez.
- 18–22: 2004, sıcak su sondajı.
- 22–28: 2005, ilk kablonun ve 60 sensörün indirilmesi.
- 28–32,5: tek kablo yanında iki ayrı yukarı yönlü müon olayı. Temsili seçimlerde 35 ve 50 sensör kullanılır; tarihsel olayların tam yeniden üretimi değildir.
- 32,5–38: 2010, 86 kablolu/5160 sensörlü temsili dizinin tamamlanması.
- 38–44: 2013, bir yayın sayfası ve 28 olaylık örneklemi temsil eden noktalar. Bunlar 28 olayın her birinin tek başına kesin olarak astrofiziksel olduğuna ilişkin bir iddia değildir.
- 44–53: anlatının geri dönüşüyle 2011 ve 2012; iki ayrı cascade tipi ışık kaydı.
- 53–57: Bert ve Ernie özel adları.
- 57–70: iki kayıt birlikte ele alınır; atmosferik ve uzak uzay kökeni olasılıkları ortak bir bağlantı noktasından ayrılır. Soru işaretleri iki olayın tek başına kesin sonuca yetmediğini ifade eder; eşit olasılık veya belirli bir kaynak yönü ima edilmez.

Tüm harita, sondaj ve olay görselleri semboliktir. Bert ve Ernie olayları arşiv verisinden çizilmemiştir. Renkler görsel kimliği korumak içindir, ölçülmüş enerji veya geliş zamanı renk ölçeği değildir. Zaman çizgisindeki duraklar eşit görsel aralıklıdır; yıl aralıkları ölçekli değildir. Türkçe açıklama metni videoya gömülmez.

## Kaynak kontrolü

- 1987'de AMANDA çalışmalarının başlaması: https://wipac.wisc.edu/people/faculty/francis-halzen/
- Grönland testleri Ağustos 1990: https://icecube.wisc.edu/news/detector/2013/08/early-icecube-tests-greenland-1990/
- 1999 teklifi, 2004 inşa dönemi, 18 Aralık 2010 tamamlanması ve 2013 bulguları: https://icecube.wisc.edu/about-us/icecube10/timeline/
- İlk kablo 2004–2005 yaz sezonu, Şubat 2005'ten veri; 35 ve 50 DOM'da kaydedilen iki yukarı yönlü olay: https://icecube.wisc.edu/wp-content/uploads/2021/01/2006.performance.pdf (Bölüm 4.2).
- Bert ve Ernie: https://arxiv.org/abs/1304.5356
- 2013 örneklemi: https://arxiv.org/abs/1311.5238

Kaynaklar inşaat başlangıcını farklı kapsamlarla Aralık 2003 veya 2004 sezonu diye ifade edebiliyor. Görsel, metindeki “2004 civarında” ifadesini izler. 1990, test yılıdır; sonuçların Nature'da yayımlanması 1991'dir.

Kaynak kod: animation/icecube_samples/render_history.py
Yeniden üretim: render_history.py --scale 2
