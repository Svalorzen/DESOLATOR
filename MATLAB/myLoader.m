function [P, R] = myLoader (filename)

fid = fopen(filename);

P = zeros(96, 96, 2);
R = zeros(96, 96, 2);

for i=1:96
    for j=1:96
        [M, C] = fscanf(fid, '%d %d %f %f', 4);
        disp(M);
        if ( C ~= 4 )
            disp('wat');
        end
            
        P(i,j,1) = M(1);
        P(i,j,2) = M(2);
        R(i,j,1) = M(3);
        R(i,j,2) = M(4);
    end
end

fclose(fid);

% Normalize
for i=1:96
    attackSum = sum(P(i, :, 1));
    fleeSum = sum(P(i, :, 2));
    for j=1:96
        % Fix reward
        if ( P(i, j, 1) ~= 0 )
            R(i, j, 1) = R(i, j, 1) / P(i, j, 1);
        end
        if ( P(i, j, 2) ~= 0 )
            R(i, j, 2) = R(i, j, 2) / P(i, j, 2);
        end
        
        if ( attackSum == 0 )
            if ( i == j )
                P(i, j, 1) = 1;
                P(i, j, 2) = 1;
            else
                P(i, j, 1) = 0;
                P(i, j, 2) = 0;
            end
        else
            P(i, j, 1) = P(i, j, 1) / attackSum;
            P(i, j, 2) = P(i, j, 2) / fleeSum;
        end
    end
end
