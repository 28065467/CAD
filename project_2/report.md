## Initial greedy cluster
使用greedy algorithm因為buffer的fanout越大其分攤下來的cost就越小，所以將整個pin腳視作集合，每次從中取出盡量多且滿足length要求的pin並以最大的buffer做連接，剩下的無法的納入的pin就把他單獨用一buffer連接![alt text](/home/lin/CAD/project_2/Naive_Greedy/image.png)
因為skew實際上最大值=Source distance - fartest pin，所以為了使最後的skew變小，我們可以犧牲一點cost來讓每個pin的T都盡量接近max(source,pin)，也就是DME的概念，所以此方法的CLUSTERING跟greedy的不一樣會挑選一個讓在此cluster內的max-min最小的位置![alt text](/home/lin/CAD/project_2/DME/image.png)

## k-means clustering
由於greedy cluster是一群一群依序成群,所以先挑的群可以挑最近的pin因而形成更加緊密的群,但這樣後面的群就會像是硬湊在一起或只有一個點,而k-means會反覆調整群的組成,這樣可以使群的大小盡量相同,所以各層子樹的delay也會比較接近
k = ceil(n/buffer_max_Fanout)
![alt text](/home/lin/CAD/project_2/kmeans/image.png)
k = greedy clustering average size
![alt text](/home/lin/CAD/project_2/kmeans/image_kmean_update.png)
## Local Search
對score影響最大的為skew,所以只要找到T_max/min所屬的sink,並對它們附近進行如移動buffer/換buffer type或移動到另一buffer的clock tree下等等的操作就可以提昇score.
因為在k-means加上averger size clustering就已經在預設的輸入中得到理論最高分了,所以之後的方法都會利用python來產生隨機的測試資料,並且以固定random seed來方便比較效果
![alt text](/home/lin/CAD/project_2/local_search/image.png)