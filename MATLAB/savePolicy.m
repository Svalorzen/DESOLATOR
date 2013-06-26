function savePolicy(P)
    fid = fopen('policy.data', 'w')
    
    for i=1:96
       fprintf(fid, '%d %d\n',i-1, P(i) -1); 
    end

    fclose(fid);
end