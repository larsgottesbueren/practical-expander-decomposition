suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file) %>% filter(near(phi, 0.005))

df$strategy <- gsub("default", "Original", df$strategy)
df$strategy <- gsub("balanced", "Balanced", df$strategy)

plot <-
    ggplot(df, aes(x=factor(type), y=balance, fill=strategy)) +
    geom_bar(stat="summary", fun="mean", position="dodge") +
    labs(y="Balance",
         x="Graph name",
         fill="Strategy") +
    theme(axis.text.x = element_text(size = 6, angle = 90, vjust = 0.5, hjust=1)) +
    facet_grid(targetbalance ~ .)

ggsave(output_file, plot)
