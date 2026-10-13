# Skew
## Initial greedy cluster
使用greedy algorithm因為buffer的fanout越大其分攤下來的cost就越小，所以將整個pin腳視作集合，每次從中取出盡量多且滿足length要求的pin並以最大的buffer做連接，剩下的無法的納入的pin就把他單獨用一buffer連接![alt text](/home/lin/CAD/project_2/Naive_Greedy/image.png)
## DME
因為skew實際上最大值=Source distance - fartest pin，所以為了使最後的skew變小，我們可以犧牲一點cost來讓每個pin的T都盡量接近max(source,pin)，也就是DME的概念，所以此方法的CLUSTERING跟greedy的不一樣會挑選一個讓在此cluster內的max-min最小的位置![alt text](/home/lin/CAD/project_2/DME/image.png)

## k-means clustering
由於greedy cluster是一群一群依序成群,所以先挑的群可以挑最近的pin因而形成更加緊密的群,但這樣後面的群就會像是硬湊在一起或只有一個點,而k-means會反覆調整群的組成,這樣可以使群的大小盡量相同,所以各層子樹的delay也會比較接近
k = ceil(n/buffer_max_Fanout)
![alt text](/home/lin/CAD/project_2/kmeans/image.png)
k = greedy clustering average size
![alt text](/home/lin/CAD/project_2/kmeans/image_kmean_update.png)
## Local Search
skew只被T_max/min影響,所以只要找到T_max/min所屬的sink,並對它們附近進行如移動buffer/換buffer type或移動到另一buffer的clock tree下等等的操作就可以提昇score.
因為在k-means加上averger size clustering就已經在預設的輸入中得到的skew已經是理論最低了,所以之後的方法都會利用python來產生隨機的測試資料,並且以固定random seed來方便比較效果
![alt text](/home/lin/CAD/project_2/local_search/image.png)

# Cost
## Dynamic Cluster size
前面都是用k-means當作分群法,現在針對平均群大小乘上一個倍率再算k,如果群大一點那buffer cost就會變小但線長就會變長,反之,群變小那線長短而buffer cost大.但最好的數值無法確定,所以只能用時間多跑幾組換取分數的提昇
## Repeater Improvement
把那些用來延伸線長的reapeater(只有一個child的buffer)，在他們附近找可能的pin並把他們連接在一起來現場線長與可能的鋅buffer cost

# Run time
大多數的時間都用來做分群建樹了,不論是k-means或greedy分群兩者都大致要o(n^2)的時間
利用網格化可以避免每次到新的PIN時都要重新計算距離(也就是O(n^2)的來源)這樣,改變後只要評估大概的距離就可以用他來在網格上挑出符合條件的點,且只要做一次就可以應用在所有pin上,因此降低時間複雜度到O(n^2)
![alt text](/home/lin/CAD/project_2/grid.png)