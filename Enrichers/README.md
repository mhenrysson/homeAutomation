# Enrichers
Enrichers for MQTT broker.

Specify a topic prefix for each enricher and it will subscribe to \<prefix\>/#, enrich the JSON content and publish the enriched message to the topic remainder, i.e. with original topic \<prefix\>/\<suffix\> the enriched message will be published in topic \<suffix\>. Multiple enrichers can be specified by using chained prefixes, e.g. \<enricher1-prefix\>/\<enricher2-prefix\>/../\<final-topic\>.

#### addtime
Adds a timestamp to json message   
TODO: Move hardcoded configurations to config file.
