./jptrsmorph -morph image_test/mask_o_mask.txt -key 8966 image_test/mask_o.jpg image_test/mask_o_sub.jpg image_test/mask_p.jpg image_test/mask_p_emb.jpg
./jptrsmorph -remorph -copy all -key 8966 image_test/mask_p_emb.jpg image_test/mask_p_rec.jpg image_test/mask_p_sub.jpg

#./jptrsmorph -morph image2/maskArray_146.txt image2/146.jpg image2/146_o_sub.jpg image2/146.jpg image2/146_p_emb.jpg
#./jptrsmorph -remorph -copy all image2/146_p_emb.jpg image2/146_p_rec.jpg image2/146_p_sub.jpg
#
#./jptrsmorph -morph image2/park.txt -key 8966 image2/park.jpg image2/park_sub.jpg image2/park.jpg image2/park_emb.jpg
#./jptrsmorph -remorph -copy all -key 8966 image2/park_emb.jpg image2/park_rec.jpg image2/park_re_sub.jpg